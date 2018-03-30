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

// Shader storage buffer objects
layout(std430, binding = 0) readonly buffer LightBuffer {
	PointLight data[];
} lightBuffer;

layout(std430, binding = 1) readonly buffer VisibleLightIndicesBuffer {
	VisibleIndex data[];
} visibleLightIndicesBuffer;

// Uniforms
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;
uniform int numberOfTilesX;
uniform int tileSize;

out vec4 fragColor;

// Attenuate the point light intensity
float attenuate(vec3 lightDirection, float radius) {
	float cutoff = 0.5;
	float attenuation = dot(lightDirection, lightDirection) / (100.0 * radius);
	attenuation = 1.0 / (attenuation * 15.0 + 1.0);
	attenuation = (attenuation - cutoff) / (1.0 - cutoff);

	return clamp(attenuation, 0.0, 1.0);
}

void main() {
	// Determina em que quadrado o pixel está
	ivec2 location = ivec2(gl_FragCoord.xy);
	ivec2 tileID = location / ivec2(tileSize, tileSize);
	uint index = tileID.y * numberOfTilesX + tileID.x;

	// Obtém cor e normal do normal map
	vec4 base_diffuse = texture(texture_diffuse1, fragment_in.textureCoordinates);
	vec4 base_specular = texture(texture_specular1, fragment_in.textureCoordinates);
	vec3 normal = texture(texture_normal1, fragment_in.textureCoordinates).rgb;

	//Expande a normal
	normal = normalize(normal * 2.0 - 1.0);
	vec4 color = vec4(0.0, 0.0, 0.0, 1.0);

	vec3 viewDirection = normalize(fragment_in.tangentViewPosition - fragment_in.tangentFragmentPosition);

	// The offset is this tile's position in the global array of valid light indices.
	// Loop through all these indices until we hit max number of lights or the end (indicated by an index of -1)
	// Calculate the lighting contribution from each visible point light
	uint offset = index * 1024;
	for (uint i = 0; i < 1024 && visibleLightIndicesBuffer.data[offset + i].index != -1; i++) {
		uint lightIndex = visibleLightIndicesBuffer.data[offset + i].index;
		PointLight light = lightBuffer.data[lightIndex];

		vec4 lightColor = light.color;
		vec3 tangentLightPosition = fragment_in.TBN * light.position.xyz;
		float lightRadius = light.paddingAndRadius.w;

		// Calcula atenuação da luz na direção pre-normalizada
		vec3 lightDirection = tangentLightPosition - fragment_in.tangentFragmentPosition;
		float attenuation = attenuate(lightDirection, lightRadius);

		// Normalize the light direction and calculate the halfway vector
		lightDirection = normalize(lightDirection);
		vec3 halfway = normalize(lightDirection + viewDirection);

		// Calcula componentes difusa e especular da irradiancia e acumula a cor
		float diffuse = max(dot(lightDirection, normal), 0.0);
		// How do I change the material propery for the spec exponent? is it the alpha of the spec texture?
		float specular = pow(max(dot(normal, halfway), 0.0), 32.0);

		//Hack para contornar quando luz especular ainda fica na cena quando a luz passa pelo objeto
		if (diffuse == 0.0) {
			specular = 0.0;
		}

		vec3 irradiance = lightColor.rgb * ((base_diffuse.rgb * diffuse) + (base_specular.rgb * vec3(specular))) * attenuation;
		color.rgb += irradiance;
	}

	color.rgb += base_diffuse.rgb * 0.08;

	// Hack para descartar fragmento com transparência
	if (base_diffuse.a <= 0.2) {
		discard;
	}
	
	fragColor = color;
}
