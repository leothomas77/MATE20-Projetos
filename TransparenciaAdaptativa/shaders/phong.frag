#version 430

#extension GL_ARB_shader_image_load_store : enable
#extension GL_ARB_shader_atomic_counters : enable

const int WIDTH  = 600;
const int HEIGHT = 600;

coherent uniform layout(rgba32f) image2DArray mapaABuffer;
coherent uniform layout(r32ui) uimage2D contadorABuffer;
coherent uniform layout(r32f) image2DArray depthBuffer;

uniform vec3 uLPos;
uniform vec3 uCamPos;

in vec3 vNormal; 
in vec3 vPosW;
out vec4 outFragColor;

vec4 colorPhong();

void main(void) {

	ivec2  coords = ivec2(gl_FragCoord.xy);

	if(coords.x >= 0 && coords.y >= 0 && coords.x < WIDTH && coords.y < HEIGHT) {
		uint abidx = imageAtomicAdd(contadorABuffer, coords, 1);
		
		vec4 abuffval;
		vec3 col;

		float depth = gl_FragCoord.z;

		abuffval = colorPhong();

		//armazena fragmento no abuffer
		imageStore(mapaABuffer, ivec3(coords, abidx), abuffval);

		//armazena o z deste fragmento no depthBuffer
		vec4 depthRGBA;
		
		depthRGBA.r = vPosW.z;
		imageStore(depthBuffer, ivec3(coords, abidx), depthRGBA); //

		outFragColor = colorPhong();
	} 

	discard;

} 	

vec4 colorPhong() {
		vec4 lColor		= vec4(1.0, 0.3, 0.2, 0.2); 
		vec4 matAmb		= vec4(1.0, 1.0, 1.0, 0.5); 
		vec4 matDif 	= vec4(0.3, 1.0, 0.6, 1.0); 
		vec4 matSpec	= vec4(1.0, 1.0, 1.0, 1.0);
	
		vec4 ambient = vec4(lColor.rgb * matAmb.rgb, matAmb.a); 

		vec3 vL = normalize(uLPos - vPosW); 
		float cTeta = max(dot(vL, vNormal), 0.0); 
			
		vec4 diffuse = vec4(lColor.rgb * matDif.rgb * cTeta, matDif.a); 

		vec3 vV = normalize(uCamPos - vPosW); 
		vec3 vR = normalize(reflect(-vL, vNormal)); 
		float cOmega = max(dot(vV, vR), 0.0);
		vec4 specular = vec4(lColor.rgb * matSpec.rgb * pow(cOmega,20.0), matSpec.a); 
	
		return clamp(ambient + diffuse + specular, 0.0, 1.0); 
}


//