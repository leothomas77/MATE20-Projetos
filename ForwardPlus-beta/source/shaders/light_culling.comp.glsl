#version 430

struct PointLight {
	vec4 color;
	vec4 position;
	vec4 paddingAndRadius;
};

struct VisibleIndex {
	int index;
};

// Area de trabalho do shader
layout(std430, binding = 0) readonly buffer LightBuffer {
	PointLight data[];
} lightBuffer;

layout(std430, binding = 1) writeonly buffer VisibleLightIndicesBuffer {
	VisibleIndex data[];
} visibleLightIndicesBuffer;

// Uniforms
uniform sampler2D depthMap;
uniform mat4 view;
uniform mat4 projection;
uniform ivec2 screenSize;
uniform int lightCount;
uniform int tileSize;

// Armazenamento compartilhado entre todas as threads
// Valores compartilhados entre todas threads do grupo de threads
shared uint minDepthInt;
shared uint maxDepthInt;
shared uint visibleLightCount;
shared vec4 frustumPlanes[6];

// Valores compartilhados para indices das luzes visíveis. Serão escritos no buffer ao final
shared int visibleLightIndices[1024];
shared mat4 viewProjection;


// http://www.dice.se/news/directx-11-rendering-battlefield-3/

#define TILE_SIZE 16
layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE, local_size_z = 1) in;

bool intersectSphere(vec4 light, vec4 plane);

void main() {
	ivec2 location = ivec2(gl_GlobalInvocationID.xy);
	ivec2 itemID = ivec2(gl_LocalInvocationID.xy);
	ivec2 tileID = ivec2(gl_WorkGroupID.xy);
	ivec2 tileNumber = ivec2(gl_NumWorkGroups.xy);
	uint index = tileID.y * tileNumber.x + tileID.x;

	// Valores iniciais para contadores e variáveis compartilhadas
	if (gl_LocalInvocationIndex == 0) {
		minDepthInt = 0xFFFFFFFF;
		maxDepthInt = 0;
		visibleLightCount = 0;
		viewProjection = projection * view;
	}

	barrier(); // sincroniza todas as threads com a barreira

	// Step 1: Calcula depth mínimo e máximo para este quadro
	float maxDepth, minDepth;
	vec2 text = vec2(location) / screenSize;
	float depth = texture(depthMap, text).r;
	// Linearização do depth considerando a projeção
	depth = (0.5 * projection[3][2]) / (depth + 0.5 * projection[2][2] - 0.5);

	// Converte o depth para inteiro para realizar operação atômica de comparação entre threads
	uint depthInt = floatBitsToUint(depth);
	atomicMin(minDepthInt, depthInt);
	atomicMax(maxDepthInt, depthInt);

	barrier();

	// Step 2: Calcula frustums
	if (gl_LocalInvocationIndex == 0) {
		// Converte depth min e max para float novamente
		minDepth = uintBitsToFloat(minDepthInt);
		maxDepth = uintBitsToFloat(maxDepthInt);

		// Parametro para cálculo de cada frustum de acordo com posição do quadro
		vec2 negativeStep = (2.0 * vec2(tileID)) / vec2(tileNumber);
		vec2 positiveStep = (2.0 * vec2(tileID + ivec2(1, 1))) / vec2(tileNumber);

		// Set up starting values for planes using steps and min and max z values
		frustumPlanes[0] = vec4(1.0, 0.0, 0.0, 1.0 - negativeStep.x); // Left
		frustumPlanes[1] = vec4(-1.0, 0.0, 0.0, -1.0 + positiveStep.x); // Right
		frustumPlanes[2] = vec4(0.0, 1.0, 0.0, 1.0 - negativeStep.y); // Bottom
		frustumPlanes[3] = vec4(0.0, -1.0, 0.0, -1.0 + positiveStep.y); // Top
		frustumPlanes[4] = vec4(0.0, 0.0, -1.0, -minDepth); // Near
		frustumPlanes[5] = vec4(0.0, 0.0, 1.0, maxDepth); // Far

		// Transforma os planos Left Right Bottom Top
		for (uint i = 0; i < 4; i++) {
			frustumPlanes[i] *= viewProjection;
			frustumPlanes[i] /= length(frustumPlanes[i].xyz);
		}

		// Transforma  Near e Far
		frustumPlanes[4] *= view;
		frustumPlanes[4] /= length(frustumPlanes[4].xyz);
		frustumPlanes[5] *= view;
		frustumPlanes[5] /= length(frustumPlanes[5].xyz);
	}

	barrier();

	// Step 3: Corte de luzes (cull)
	// Parallelize the threads against the lights now.
	// Can handle 256 simultaniously. Anymore lights than that and additional passes are performed
	uint threadCount = TILE_SIZE * TILE_SIZE;
	uint passCount = (lightCount + threadCount - 1) / threadCount;
	for (uint i = 0; i < passCount; i++) {
		// Get the lightIndex to test for this thread / pass. If the index is >= light count, then this thread can stop testing lights
		uint lightIndex = i * threadCount + gl_LocalInvocationIndex;
		if (lightIndex >= lightCount) {
			break;
		}

		// Obtém posição e raio da esfera que representa ponto de luz
		vec4 position = lightBuffer.data[lightIndex].position;
		float radius = lightBuffer.data[lightIndex].paddingAndRadius.w;

		// Verifica interseção da luz (no caso uma esfera) com frustum
		// http://www.flipcode.com/archives/Frustum_Culling.shtml
		float distance = 0.0;
		for (uint j = 0; j < 6; j++) {
			distance = dot(position, frustumPlanes[j]) + radius;

			// If one of the tests fails, then there is no intersection
			if (distance <= 0.0) {
				break;
			}
		}

		// Se foi maior que zero, então tem interseção do frustum com a luz
		if (distance > 0.0) {
			// Armazena a o índice da luz visível, incrementa o contador de luzes
			uint offset = atomicAdd(visibleLightCount, 1);
			visibleLightIndices[offset] = int(lightIndex);
		}
	}

	barrier();

	// One thread should fill the global light buffer
	if (gl_LocalInvocationIndex == 0) {
		uint offset = index * 1024; // Determine bosition in global buffer
		for (uint i = 0; i < visibleLightCount; i++) {
			visibleLightIndicesBuffer.data[offset + i].index = visibleLightIndices[i];
		}

		if (visibleLightCount != 1024) {
			// Unless we have totally filled the entire array, mark it's end with -1
			// Final shader step will use this to determine where to stop (without having to pass the light count)
			visibleLightIndicesBuffer.data[offset + visibleLightCount].index = -1;
		}
	}
}
