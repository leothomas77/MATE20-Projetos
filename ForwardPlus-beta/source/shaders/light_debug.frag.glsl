#version 430

in VERTEX_OUT{
	vec3 fragmentPosition;
	vec2 textureCoordinates;
	mat3 TBN;
	vec3 tangentViewPosition;
	vec3 tangentFragmentPosition;
} fragment_in;

struct PointLight {
	vec4 color;
	vec4 position;
	vec4 paddingAndRadius;
};

struct VisibleIndex {
	int index;
};

// Area de armazenamento do shader
layout(std430, binding = 0) readonly buffer LightBuffer{
	PointLight data[];
} lightBuffer;

layout(std430, binding = 1) readonly buffer VisibleLightIndicesBuffer{
	VisibleIndex data[];
} visibleLightIndicesBuffer;

uniform int numberOfTilesX;
uniform int totalLightCount;
uniform int tileSize

out vec4 fragColor;

void main() {
	// Determina em qual quadro o pixel pertence
	ivec2 location = ivec2(gl_FragCoord.xy);
	ivec2 tileID = location / ivec2(tileSize, tileSize);
	uint index = tileID.y * numberOfTilesX + tileID.x;

	uint offset = index * 1024;
	uint i;
	for (i = 0; i < 1024 && visibleLightIndicesBuffer.data[offset + i].index != -1; i++);

	float ratio = float(i) / float(totalLightCount);
	fragColor = vec4(vec3(ratio, ratio, ratio), 1.0);
}
//
