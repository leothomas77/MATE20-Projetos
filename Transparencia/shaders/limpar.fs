#version 430  
#extension GL_ARB_shader_image_load_store : enable
#extension GL_ARB_shader_atomic_counters : enable

coherent uniform layout(location = 0, rgba32f) image2DArray mapaABuffer;
coherent uniform layout(location = 1, r32ui) uimage2D contadorABuffer;
coherent uniform layout(location = 2, r32f) image2DArray depthBuffer;

layout(pixel_center_integer) in vec4 gl_FragCoord;

void main(void) {
	//Obtém x e y de cada fragmento
	ivec2 coords = ivec2(gl_FragCoord.xy);

	//Limpa contador para as coordenadas x e y deste fragmento
	imageStore(contadorABuffer, coords, ivec4(0));
		
	//Preenche a primeira camada com valor do BACKGROUND
	imageStore(mapaABuffer, ivec3(coords, 0), vec4(0.0f, 0.0f, 0.0f, 0.0f));

}
//