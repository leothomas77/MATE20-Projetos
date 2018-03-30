
#include "main.h"

void InitGLFW(int argc, char* argv[]) {
	if (!glfwInit()) {
		throw std::runtime_error("glfwInit falhou");
	}

	// Configuração do GLFW
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	gWindow = glfwCreateWindow(SCREEN_SIZE.x, SCREEN_SIZE.y, "Forward Plus 0.1", NULL, NULL);
	if (!gWindow) {
		throw std::runtime_error("Erro em glfwCreateWindow. Precisa de OpenGL 4.3!");
	}
	glfwMakeContextCurrent(gWindow);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		throw std::runtime_error("glewInit falhou");
	}

	// Mostra informações sobre ambiente
	std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
	std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

	// Verifica versão disponível de OpenGL
	if (!GLEW_VERSION_4_3) {
		throw std::runtime_error("OpenGL 4.3 API não disponível.");
	}

	// Habilita funções do OpenGL
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);

	// Configura eventos de mouse e teclado
	glfwSetKeyCallback(gWindow, KeyCallback);
	glfwSetCursorPosCallback(gWindow, MouseCallback);
	glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void InitScene() {
	// Divide a tela em uma quantidade de 16 fatias na direção x e y de acordo com o tamanho da tela em pixels
	// O tamanho de cada retângulo vai variar de acordo com a tela
	//Cada retângulo vai ser tabalhado em um workgroup separado (todos rodando em paralelo)
	workGroupsX = (SCREEN_SIZE.x + (SCREEN_SIZE.x % TILE_SIZE)) / TILE_SIZE;
	workGroupsY = (SCREEN_SIZE.y + (SCREEN_SIZE.y % TILE_SIZE)) / TILE_SIZE;
	size_t numberOfTiles = workGroupsX * workGroupsY;

	// Geração dos buffers de trabalho para os shaders
	glGenBuffers(1, &lightBuffer);
	glGenBuffers(1, &visibleLightIndicesBuffer);

	// Bind para buffer de luzes
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_LIGHTS * sizeof(PointLight), 0, GL_DYNAMIC_DRAW);

	// Bind para buffer de índices de luzes visíveis em cada quadrado de tela
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, visibleLightIndicesBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numberOfTiles * sizeof(VisibleIndex) * MAX_LIGHTS, 0, GL_STATIC_DRAW);

	// Iniclialização das luzes
	SetupLights();

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

//Randomização da posição das luzes
glm::vec3 RandomPosition(uniform_real_distribution<> dis, mt19937 gen) {
	glm::vec3 position = glm::vec3(0.0);
	for (int i = 0; i < 3; i++) {
		float min = LIGHT_MIN_BOUNDS[i];
		float max = LIGHT_MAX_BOUNDS[i];
		position[i] = (GLfloat)dis(gen) * (max - min + 1) + min;
	}

	return position;
}

void SetupLights() {
	if (lightBuffer == 0) {
		return;
	}
	
	random_device rd;
	mt19937 gen(rd());
	uniform_real_distribution<> dis(0, 1); // Gera uma distribuição randomica de números reais no intervalo de [0, 1)

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightBuffer);
	PointLight *pointLights = (PointLight*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);

	for (int i = 0; i < NUM_LIGHTS; i++) {
		PointLight &light = pointLights[i];
		light.position = glm::vec4(RandomPosition(dis, gen), 1.0f);
		light.color = glm::vec4(1.0f + dis(gen), 1.0f + dis(gen), 1.0f + dis(gen), 1.0f);
		light.paddingAndRadius = glm::vec4(glm::vec3(1.0f), LIGHT_RADIUS);
	}

	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void UpdateLights() {
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightBuffer);
	PointLight *pointLights = (PointLight*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);

	for (int i = 0; i < NUM_LIGHTS; i++) {
		PointLight &light = pointLights[i];
		float min = LIGHT_MIN_BOUNDS.y;
		float max = LIGHT_MAX_BOUNDS.y;
		//varia a posicao y
		light.position.y = fmod((light.position.y + (LIGHT_STEP * deltaTime) - min + max), max) + min;
	}

	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

// Movimento da câmera com teclado
void Movement() {
	if (keys[GLFW_KEY_W]) {
		camera.ProcessKeyboard(FORWARD, deltaTime);
	}

	if (keys[GLFW_KEY_S]) {
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	}

	if (keys[GLFW_KEY_A]) {
		camera.ProcessKeyboard(LEFT, deltaTime);
	}

	if (keys[GLFW_KEY_D]) {
		camera.ProcessKeyboard(RIGHT, deltaTime);
	}
}

static void KeyCallback(GLFWwindow *window, int key, int scanCode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key >= 0 && key <= 1024) {
		if (action == GLFW_PRESS) {
			keys[key] = true;
		}
		else if (action == GLFW_RELEASE) {
			keys[key] = false;
			keysPressed[key] = false;
		}
	}
}

//Movimento da câmera com mouse
static void MouseCallback(GLFWwindow *window, double x, double y) {
	if (firstMouse) {
		lastX = (GLfloat)x;
		lastY = (GLfloat)y;
		firstMouse = false;
	}

	GLfloat xOffset = (GLfloat)x - lastX;
	GLfloat yOffset = lastY - (GLfloat)y;

	lastX = (GLfloat)x;
	lastY = (GLfloat)y;

	camera.ProcessMouseMovement(xOffset, yOffset);
}

//Desenha o quadro da tela
//No Deferred shading os pixels serão gerados em uma textura passo a passo. No passo final, irá desenhar na tela
//um conjunto de triângulos texturizados com estes pixels
void DrawQuad() {
	if (quadVAO == 0) {
		GLfloat quadVertices[] = {
			-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
			1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};

		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	}

	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}


int main(int argc, char **argv) {
	InitGLFW(argc, argv);

	// TODO: Para executar, deve modificar todos estes caminhos para o caminho absoluto

	char *dir = "C:\\Users\\gita\\git\\MATE20-Projetos\\Forward-beta";

	Shader depthShader("C:\\Users\\gita\\git\\MATE20-Projetos\\ForwardPlus-beta\\source\\shaders\\depth.vert.glsl",
		"C:\\Users\\gita\\git\\MATE20-Projetos\\ForwardPlus-beta\\source\\shaders\\depth.frag.glsl", NULL);
	Shader lightCullingShader("C:\\Users\\gita\\git\\MATE20-Projetos\\ForwardPlus-beta\\source\\shaders\\light_culling.comp.glsl");

#if defined(DEPTH_DEBUG)
	Shader depthDebugShader("C:\\Users\\gita\\git\\MATE20-Projetos\\ForwardPlus-beta\\source\\shaders\\depth_debug.vert.glsl",
		"C:\\Users\\gita\\git\\MATE20-Projetos\\ForwardPlus-beta\\source\\shaders\\depth_debug.frag.glsl", NULL);
#elif defined(LIGHT_DEBUG)
	Shader lightDebugShader("C:\\Users\\gita\\git\\MATE20-Projetos\\ForwardPlus-beta\\source\\shaders\\light_debug.vert.glsl",
		"C:\\Users\\gita\\git\\MATE20-Projetos\\ForwardPlus-beta\\source\\shaders\\light_debug.frag.glsl", NULL);
#else
	Shader lightAccumulationShader("C:\\Users\\gita\\git\\MATE20-Projetos\\ForwardPlus-beta\\source\\shaders\\light_accumulation.vert.glsl",
		"C:\\Users\\gita\\git\\MATE20-Projetos\\ForwardPlus-beta\\source\\shaders\\light_accumulation.frag.glsl", NULL);
	Shader hdrShader("C:\\Users\\gita\\git\\MATE20-Projetos\\ForwardPlus-beta\\source\\shaders\\hdr.vert.glsl",
		"C:\\Users\\gita\\git\\MATE20-Projetos\\ForwardPlus-beta\\source\\shaders\\hdr.frag.glsl", NULL);
#endif

	//Criação do mapa de depth o passo 1
	//Armazenamento dos depths em textura
	GLuint depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);

	GLuint depthMap;
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCREEN_SIZE.x, SCREEN_SIZE.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	GLfloat borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

#if defined(DEPTH_DEBUG)
#elif defined(LIGHT_DEBUG)
#else
	
	//Cria framebuffer para tratar HDR (em formato de textura)
	GLuint hdrFBO;
	glGenFramebuffers(1, &hdrFBO);

	GLuint colorBuffer;
	glGenTextures(1, &colorBuffer);
	glBindTexture(GL_TEXTURE_2D, colorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCREEN_SIZE.x, SCREEN_SIZE.y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// It will also need a depth component as a render buffer, attached to the hdrFBO
	GLuint rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCREEN_SIZE.x, SCREEN_SIZE.y);

	glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif

	// Carrega o modelo sponza
	// http://www.crytek.com/cryengine/cryengine3/downloads
		
	Model sponzaModel("C:\\Users\\gita\\git\\MATE20-Projetos\\ForwardPlus-beta\\crytek-sponza\\sponza.obj");

	// Inicialização da cena
	InitScene();

	// Gera a matriz model view. Aplica escala
	glm::mat4 model;
	model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));

	// Faz o bind com as variáveis do tipo uniform para cada shader
	depthShader.Use();
	glUniformMatrix4fv(glGetUniformLocation(depthShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));

	lightCullingShader.Use();
	glUniform1i(glGetUniformLocation(lightCullingShader.Program, "lightCount"), NUM_LIGHTS);
	glUniform2iv(glGetUniformLocation(lightCullingShader.Program, "screenSize"), 1, &SCREEN_SIZE[0]);


#if defined(DEPTH_DEBUG)
	depthDebugShader.Use();
	glUniformMatrix4fv(glGetUniformLocation(depthDebugShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniform1f(glGetUniformLocation(depthDebugShader.Program, "near"),  NEAR_PLANE);
	glUniform1f(glGetUniformLocation(depthDebugShader.Program, "far"),  FAR_PLANE);
#elif defined(LIGHT_DEBUG)
	lightDebugShader.Use();
	glUniformMatrix4fv(glGetUniformLocation(lightDebugShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniform1i(glGetUniformLocation(lightDebugShader.Program, "totalLightCount"), NUM_LIGHTS);
	glUniform1i(glGetUniformLocation(lightDebugShader.Program, "numberOfTilesX"), workGroupsX);
	glUniform1i(glGetUniformLocation(lightDebugShader.Program, "tileSize"), TILE_SIZE);
#else
	lightAccumulationShader.Use();
	glUniformMatrix4fv(glGetUniformLocation(lightAccumulationShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniform1i(glGetUniformLocation(lightAccumulationShader.Program, "numberOfTilesX"), workGroupsX);
	glUniform1i(glGetUniformLocation(lightAccumulationShader.Program, "tileSize"), TILE_SIZE);
#endif

	// Set viewport dimensions and background color
	glViewport(0, 0, SCREEN_SIZE.x, SCREEN_SIZE.y);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	// Loop principal
	while (!glfwWindowShouldClose(gWindow)) {
		// Calcula tempo para movimentação das luzes 
		GLfloat currentFrame = (GLfloat)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Tratamento de eventos
		glfwPollEvents();
		Movement();

		// Movimenta luzes
		UpdateLights();

		// Calcula matrizes para posição da câmera
		glm::mat4 projection = glm::perspective(camera.zoom, (float)SCREEN_SIZE.x / (float)SCREEN_SIZE.y, NEAR_PLANE, FAR_PLANE);
		glm::mat4 view = camera.GetViewMatrix();
		glm::vec3 cameraPosition = camera.position;

		// Step 1: Mapeia o depth da cena
		depthShader.Use();
		glUniformMatrix4fv(glGetUniformLocation(depthShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(depthShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
		
		// Bind no depth buffer e desenha nele
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		sponzaModel.Draw(depthShader);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Step 2: Corte de luzes - compute chader
		lightCullingShader.Use();
		glUniformMatrix4fv(glGetUniformLocation(lightCullingShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(lightCullingShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));

		// Bind do depth buffer no compute shader, posição 4 (demais posições já estão sendo usadas pelos objetos da cena)
		glActiveTexture(GL_TEXTURE4);
		glUniform1i(glGetUniformLocation(lightCullingShader.Program, "depthMap"), 4);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		
		// Bind do storage buffer no shader para armazenar os índices das luzes
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, visibleLightIndicesBuffer);

		// Dispara o computer shader com os valores de workgroups calculados anteriormente
		glDispatchCompute(workGroupsX, workGroupsY, 1);

		// Unbind no depth buffer
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Debug do depth shader e light shader
#if defined(DEPTH_DEBUG)
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		depthDebugShader.Use();
		glUniformMatrix4fv(glGetUniformLocation(depthDebugShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(depthDebugShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));

		sponzaModel.Draw(depthDebugShader);
#elif defined(LIGHT_DEBUG)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		lightDebugShader.Use();
		glUniformMatrix4fv(glGetUniformLocation(lightDebugShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(lightDebugShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniform3fv(glGetUniformLocation(lightDebugShader.Program, "viewPosition"), 1, &cameraPosition[0]);
		glUniform1i(glGetUniformLocation(lightDebugShader.Program, "tileSize"), TILE_SIZE);

		sponzaModel.Draw(lightDebugShader);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
#else
		// Step 3: Calcula a contribuição das luzes computadas
		// Renderiza em framebuffer. Utiliza o recurso de HDR em ponto flutuante
		glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		lightAccumulationShader.Use();
		glUniformMatrix4fv(glGetUniformLocation(lightAccumulationShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(lightAccumulationShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniform3fv(glGetUniformLocation(lightAccumulationShader.Program, "viewPosition"), 1, &cameraPosition[0]);
		glUniform1i(glGetUniformLocation(lightAccumulationShader.Program, "tileSize"), TILE_SIZE);

		// Blending necessita de passos adicionais
		// Por hora desabilitado
		//glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);

		sponzaModel.Draw(lightAccumulationShader);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//glDisable(GL_BLEND);
		
		// O HDR alarga o valor ente o ponto mais claro e o mais escuro da imagem usando fatores de correção
		// https://learnopengl.com/Advanced-Lighting/HDR

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Weirdly, moving this call drops performance into the floor
		hdrShader.Use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, colorBuffer);
		glUniform1f(glGetUniformLocation(hdrShader.Program, "exposure"), exposure);
		DrawQuad();

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
#endif

		glfwSwapBuffers(gWindow);
	}

	glfwTerminate();

	return 0;
}