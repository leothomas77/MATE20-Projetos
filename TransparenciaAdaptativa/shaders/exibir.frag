#version 430

#extension GL_ARB_shader_image_load_store : enable
#extension GL_ARB_shader_atomic_counters : enable

const vec4 BACKGROUND = vec4( 0.0f,  0.0f,  0.0f, 1.0f);
const uint MAX_STEPS = 8;

const float DEPTH_MIN = -1.75f;
const float DEPTH_MAX = 1.75;


const uint TAM_BUFFER  = 16;
const int WIDTH  = 600;
const int HEIGHT = 600;

layout(pixel_center_integer) in vec4 gl_FragCoord;

smooth in vec4 fragPos;

out vec4 outFragColor;

coherent uniform layout(binding = 0, rgba32f) image2DArray mapaABuffer;
coherent uniform layout(location = 1, r32ui) uimage2D contadorABuffer;
coherent uniform layout(location = 2, r32f) image2DArray depthBuffer;

//Valores brutos obtidos do shader anterior
vec4 fragmentosRGBA[TAM_BUFFER];
vec4 fragmentosDepth[TAM_BUFFER];

//Valores compactados obtidos neste shader
vec4 fragmentosRGBACompactados[TAM_BUFFER];

//Valores dos depths com steps
float depthsComSteps[MAX_STEPS];

int tamanhoCompactado;

void preencherArrays(ivec2 coords, int tamanho);

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

	if (coords.x >= 0 && coords.y >= 0 && coords.x < WIDTH && coords.y < HEIGHT){

		//Obtém quantidade de fragmentos no pixel desta posição X Y
		int quanFragmentos = int(imageLoad(contadorABuffer, coords).r);

		if(quanFragmentos > 0){
			//Preenche os 2 arrays de fragmentos nas variaveis locais
			preencherArrays(coords, quanFragmentos);

			//Ordena os fragmentos
			bubbleSort(quanFragmentos);

			//Compacta os fragmentos com a função step. Gera um novo array de fragmentos
			tamanhoCompactado = compactaFragmentos(coords, quanFragmentos);

			//Blend final
			outFragColor = blendCores(coords, tamanhoCompactado, fragmentosRGBACompactados);
			
			//outFragColor = blendCores(coords, quanFragmentos, fragmentosRGBA);

			//outFragColor = debugQuantFragmentos(quanFragmentos);

		} else {
			//Sem fragmento, não faz nada
			discard;
		}

	}

}

void preencherArrays(ivec2 coords, int tamanho) {
	for(int i=0; i < tamanho; i++){
		fragmentosRGBA[i] = imageLoad(mapaABuffer, ivec3(coords, i));
		fragmentosDepth[i] = imageLoad(depthBuffer, ivec3(coords, i));
	}
}

int compactaFragmentos(ivec2 coords, int tamanho) {
	float depthCompactado = DEPTH_MIN;
	float depthCompactadoAnt = DEPTH_MIN;

	int j = 0;

	//Percorre todos o array dos depths ordenados
	for(int i = 0; i < tamanho; i++) {
		vec4 depthRGBA = imageLoad(depthBuffer, ivec3(coords, i)); // obtém um depth
		float depth = depthRGBA.r;
		depthCompactado = funcaoStep(depth); // aplica a função de step para este depth

		//Se este depth ainda não foi armazenado no array de depths compactados
		//Caso ja tenha sido armazenado, entao vai descartar este depth e passa para o proximo
		if (depthCompactado != depthCompactadoAnt) {
			fragmentosRGBACompactados[j] = fragmentosRGBA[i]; // copia o pixel para o array dos compactados
			j++; //posiciona o array dos compactados
			depthCompactadoAnt = depthCompactado; // atualiza variavel auxiliar do depth anterior
		}

	}

	return j; // ao fim, retorna o tamanho que ficou o array compactado
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
		float depthProximo = fragmentosDepth[j].r;
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
	vec4 ret;
	if (tamanho < 2)
		ret = vec4(1.0f, 1.0f, 1.0f, 0.5f);
	else if (tamanho == 2)
		ret = vec4(1.0f, 0.0f, 0.0f, 0.5f);
	else if (tamanho > 2 && tamanho < 4)
		ret = vec4(1.0f, 0.5f, 0.0f, 0.5f);
	else if (tamanho == 4 )
		ret = vec4(0.0f, 1.0f, 0.0f, 0.5f);
	else if (tamanho > 4 ) 
		ret = vec4(0.0f, 0.0f, 1.0f, 0.5f);
	return ret;
}

void calcularSteps(float depthMin, float depthMax) {

	float step = (depthMax - depthMin) / MAX_STEPS;

	for (int i = 0; i < MAX_STEPS; i++) {
		depthsComSteps[i] = depthMin + step * i;
	}

}


float funcaoStep2(float depth) {

	if (depth < -1.0f) {
		return -1.75;
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

	if (depth < DEPTH_MIN) {
		return DEPTH_MIN;
	}

	if (depth > DEPTH_MAX) {
		return DEPTH_MAX;
	}

	//Pré calcula os steps para o fragmento
	calcularSteps(DEPTH_MIN, DEPTH_MAX);

	for (int i = 0; i < MAX_STEPS - 1; i++) {
		if (depth >= depthsComSteps[i] && depth < depthsComSteps[i + 1]) {
			depthCompactado = depthsComSteps[i];
			indiceDepthCompactado = i;
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