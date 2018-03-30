#version 430

#extension GL_ARB_shader_image_load_store : enable
#extension GL_ARB_shader_atomic_counters : enable

const vec4 BACKGROUND = vec4( 0.0f,  0.0f,  0.0f, 1.0f);
const uint MAX_STEPS = 4;

float minDepth;
float maxDepth;

const uint TAM_BUFFER  = 16;

layout(pixel_center_integer) in vec4 gl_FragCoord;

smooth in vec4 fragPos;

out vec4 outFragColor;

coherent uniform layout(binding = 0, rgba32f) image2DArray mapaABuffer;
coherent uniform layout(location = 1, r32ui) uimage2D contadorABuffer;
coherent uniform layout(location = 2, r32f) image2DArray depthBuffer;
coherent uniform layout(location = 3, r32f) bool comAT;
coherent uniform layout(location = 4, r32f) bool debug;

//Valores brutos obtidos do shader anterior
vec4 fragmentosRGBA[TAM_BUFFER];
vec4 fragmentosDepth[TAM_BUFFER];

//Valores compactados obtidos neste shader
vec4 fragmentosRGBACompactados[TAM_BUFFER];

//Valores dos depths com steps
float depthsComSteps[MAX_STEPS];

//ATENÇÃO: parâmetros por referência possuem modificador inout
//https://www.khronos.org/opengl/wiki/Core_Language_(GLSL)#Parameters
void preencherArrays(ivec2 coords, int tamanho, inout float minDepth, inout float maxDepth);
void bubbleSort(int tamanho);
vec4 blendCores(ivec2 coords, int tamanho, vec4 fragmentos[TAM_BUFFER]);
vec4 debugQuantFragmentos(int tamanho);
vec4 debugDepth(int tamanho);
float funcaoStep(float depth);
float funcaoStep2(float depth);
int compactaFragmentos(ivec2 coords, int tamanho);
void calcularSteps(float depthMin, float depthMax);

void main(void) {

	ivec2 coords = ivec2(gl_FragCoord.xy);

	//Obtém quantidade de fragmentos no pixel desta posição X Y
	int quanFragmentos = int(imageLoad(contadorABuffer, coords).r);

	if(quanFragmentos > 0) {
		//Preenche os 2 arrays de fragmentos nas variaveis locais
		preencherArrays(coords, quanFragmentos, minDepth, maxDepth);

		//Ordena os fragmentos por valor de depth
		bubbleSort(quanFragmentos);

		if (debug && comAT) {
			//Pré calcula os steps para o fragmento
			calcularSteps(minDepth, maxDepth);
			int tamanhoCompactado = compactaFragmentos(coords, quanFragmentos);
			outFragColor = debugQuantFragmentos(tamanhoCompactado);

		} else if (debug && !comAT) {
			outFragColor = debugQuantFragmentos(quanFragmentos);
		} else {
			if (comAT) {
				//Pré calcula os steps para o fragmento
				calcularSteps(minDepth, maxDepth);

				//Compacta os fragmentos com a função step. Gera um novo array de fragmentos
				int tamanhoCompactado = compactaFragmentos(coords, quanFragmentos);
			
				//Blend final
				outFragColor = blendCores(coords, tamanhoCompactado, fragmentosRGBACompactados);
			} else {
				outFragColor = blendCores(coords, quanFragmentos, fragmentosRGBA);
			}
		}

	} else {
		//Sem fragmento, não faz nada
		discard;
	}


}

//Preenche arrays de trabalho
//Determina quem é min e max depth
//ATENÇÃO: parâmetros por referência possuem modificador inout
//https://www.khronos.org/opengl/wiki/Core_Language_(GLSL)#Parameters
void preencherArrays(ivec2 coords, int tamanho, inout float minDepth, inout float maxDepth) {
	minDepth = (imageLoad(depthBuffer, ivec3(coords, 0))).r;
	maxDepth = (imageLoad(depthBuffer, ivec3(coords, 0))).r;
	for(int i=0; i < tamanho; i++) {
		fragmentosRGBA[i] = imageLoad(mapaABuffer, ivec3(coords, i));
		fragmentosDepth[i] = imageLoad(depthBuffer, ivec3(coords, i));
		
		if (fragmentosDepth[i].r < minDepth) {
			minDepth = fragmentosDepth[i].r; 
		} 
		if (fragmentosDepth[i].r > maxDepth) {
			maxDepth = fragmentosDepth[i].r; 
		}
	}
}

int compactaFragmentos(ivec2 coords, int tamanho) {

	int quantCompactados = 0;

	//Inicialização
	float depth = fragmentosDepth[0].r;			// obtém um depth
	float depthCompactado = funcaoStep(depth);	// aplica a função de step para este depth
	float depthCompactadoAnt = depthCompactado;
	fragmentosRGBACompactados[0] = fragmentosRGBA[0];
	quantCompactados++;

	//Percorre todos o array dos depths ordenados
	for(int i = 0; i < tamanho; i++) {
		depth = fragmentosDepth[i].r;
		depthCompactado = funcaoStep(depth);

		//Se este depth ainda não foi armazenado no array de depths compactados
		//Caso ja tenha sido armazenado, entao vai descartar este depth e passa para o proximo
		if (depthCompactado > depthCompactadoAnt) {
			depthCompactadoAnt = depthCompactado;
			fragmentosRGBACompactados[quantCompactados] = fragmentosRGBA[i];
			quantCompactados++;
		}
	}

	return quantCompactados; // ao fim, retorna o tamanho que ficou o array compactado
}

//Mistura as cores em um array ordenado de frente para trás
vec4 blendCores(ivec2 coords, int tamanho, vec4 fragmentos[TAM_BUFFER]) {
	
	vec4 corBlend = vec4(0.0f); 
	
	for(int i = 0; i < tamanho; i++) {
		vec4 cor;
		cor.rgb = fragmentos[i].rgb;
		cor.rgb = cor.rgb * fragmentos[i].a; // a cor vai ser atenuada pela componente alpha
		corBlend = corBlend + cor * (1.0f - corBlend.a); // a cor vai ser misturada
	
	}

	corBlend = corBlend + BACKGROUND * (1.0f - corBlend.a); // no final, mistura com background

	return corBlend;
}

//Ordena os fragmentos dos 2 arrays (depth e RGBA) pelo depth com bubble sort
void bubbleSort(int tamanho) {
  for (int i = (tamanho - 2); i >= 0; --i) {
    for (int j = 0; j <= i; ++j) {
		float depth = fragmentosDepth[j].r;
		float depthProximo = fragmentosDepth[j + 1].r;
		//Faz a troca
		if (depth > depthProximo) {
			vec4 depthTemp = fragmentosDepth[j+1];
			fragmentosDepth[j+1] = fragmentosDepth[j];
			fragmentosDepth[j] = depthTemp;

			vec4 corTemp = fragmentosRGBA[j+1];
			fragmentosRGBA[j+1] = fragmentosRGBA[j];
			fragmentosRGBA[j] = corTemp;
      }
    }
  }
}

vec4 debugQuantFragmentos(int tamanho) {
	return vec4(0.2, 0.2, 0.2, 1.0) * (tamanho + 0.3);
}

void calcularSteps(float depthMin, float depthMax) {
	float step = (depthMax - depthMin) / MAX_STEPS;

	for (int i = 0; i < MAX_STEPS; i++) {
		depthsComSteps[i] = depthMin + step * i;
	}

}


float funcaoStep2(float depth) {

	if (depth < -1.75) {
		return -1.75;
	} else if (depth > -1.5f && depth < -1.25) {
		return -1.75;
	} else if (depth > -1.5f && depth < -1.25) {
		return -1.5;
	} else if (depth > -1.25f && depth < -1.0) {
		return -1.25;
	} else if (depth >= -1.0f && depth < -0.75f) {
		return -1.0;
	} else if (depth >= -0.75f && depth < -0.5f) {
		return -0.75;
	} else if (depth >= -0.5f && depth < 0.0f) {
		return -0.5;
	} else if (depth >= 0.0f && depth < 0.25f) {
		return 0.0;
	} else if (depth >= 0.25f && depth < 0.5f) {
		return 0.25;
	} else if (depth >= 0.5f && depth < 0.75f) {
		return 0.5;
	} else if (depth >= 0.75f && depth < 1.0f) {
		return 0.75;
	} else	if (depth >= 1.0f && depth < 1.25f) {
		return 1.0;
	} else if (depth >= 1.25f && depth < 1.5f) {
		return 1.25;
	} else if (depth >= 1.5f && depth < 1.75f) {
		return 1.5;
	} else {
		return 1.75;
	}
}

float funcaoStep(float depth) {
	float depthCompactado = depth;
	int indiceDepthCompactado = 0;

	if (depth <= minDepth) {
		return minDepth;
	}

	if (depth >= maxDepth) {
		return maxDepth;
	}

	if (MAX_STEPS < 2) {
		return depthsComSteps[0];
	}

	for (int i = 0; i < MAX_STEPS - 1; i++) {
		if ((depth >= depthsComSteps[i] && depth < depthsComSteps[i + 1])) {
			depthCompactado = depthsComSteps[i];
			indiceDepthCompactado = i;
			break;
		} 
	}

	//Verifica qual o step mais adequado 
	float delta1 = depth - depthCompactado;
	float delta2 = depthsComSteps[indiceDepthCompactado + 1] - depth;

	//Toma a decisao pelo step com menor delta
	if (delta2 < delta1) {
		depthCompactado = depthsComSteps[indiceDepthCompactado + 1];
	} 

	return depthCompactado;

}
//