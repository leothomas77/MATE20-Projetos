#version 430 core

// Textura para buffer HDR
in vec2 TextureCoordinates;

// Uniforms
uniform sampler2D hdrBuffer;
uniform float exposure;

out vec4 fragColor;

// https://learnopengl.com/Advanced-Lighting/HDR

void main() {
	// HDR usado pois os valores de cores são float em uma faixa maior do que 0 .. 255
	vec3 color = texture(hdrBuffer, TextureCoordinates).rgb;
	vec3 result = vec3(1.0) - exp(-color * exposure);

	// Tonemap
	const float gamma = 2.2f;
	result = pow(result, vec3(1.0 / gamma));
	fragColor = vec4(result, 1.0);
}