#version 430
#extension GL_ARB_shader_atomic_counters : enable
#extension GL_ARB_shader_image_load_store : enable


struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
}; 

struct LuzPontual {
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;
	
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct LuzIndex {
	int index;
};

// Area de trabalho do shader
layout(std430, binding = 0) readonly buffer LightBuffer {
	LuzPontual data[];
} lightBuffer;

layout(std430, binding = 1) writeonly buffer VisibleLightIndicesBuffer {
	LuzIndex data[];
} visibleLightIndicesBuffer;

layout(std430, binding = 2) writeonly buffer VisibleLightCountBuffer {
	LuzIndex data[];
} visibleLightCountBuffer;


in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec3 FragColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform sampler2D difuseMap;

uniform vec3 viewPos;
uniform sampler2D depthMap;
uniform int scr_height;
uniform int max_luzes;

uniform Material material;


void main() {

	ivec2 fragCoords = ivec2(gl_FragCoord.xy);
	float depth = texture(depthMap, fragCoords).r;

	vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
	
	int cont = 0;

	vec3 cor = vec3(0); // para debug somente

		float radius = 0.6f;

		for (int i = 0; i < max_luzes; i++) {
			LuzPontual luz = lightBuffer.data[i];

			float distance = length(luz.position - FragPos.xyz);
			if (distance < radius) {
				int index = atomicAdd(visibleLightCountBuffer.data[fragCoords.x * scr_height + fragCoords.y].index, 1);

				visibleLightIndicesBuffer.data[fragCoords.x * scr_height * max_luzes + fragCoords.y * max_luzes + index].index  = i; 
				
				cor += vec3(0.2, 0.0, 0.0); // por para debug
			}
		}


	FragColor = cor; // cor para debug
}
