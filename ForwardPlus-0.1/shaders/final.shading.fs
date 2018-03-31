#version 430 core
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

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

uniform vec3 viewPos;
uniform Material material;
uniform int scr_height;
uniform int max_luzes;

vec3 iluminacaoPontual(LuzPontual light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{    
	ivec2 fragCoords = ivec2(gl_FragCoord.xy);

	vec3 result = vec3(0.0);
	int visibleLightCounter = visibleLightCountBuffer.data[fragCoords.x * scr_height + fragCoords.y].index; 
	if (visibleLightCounter > 0) {
		vec3 norm = normalize(Normal);
		vec3 viewDir = normalize(viewPos - FragPos);
    
		for(int i = 0; i < visibleLightCounter; i++) {
			
			int visibleLightIndice = visibleLightIndicesBuffer.data[i].index;

			result += iluminacaoPontual(lightBuffer.data[visibleLightIndice], norm, FragPos, viewDir);    
		}

		result = vec3(1.0, 0.0, 0.0);
	
	}

    
    FragColor = vec4(result, 1.0);
}

// calcula contribuição das luzes pontuais
vec3 iluminacaoPontual(LuzPontual light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}