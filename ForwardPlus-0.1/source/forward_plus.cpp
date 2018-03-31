//
// API básica para:
//	- shaders
//	- navegação com mouse e teclado
//	- câmera 
//	- arcabouço do programa 
//	- carga de textura
//	foi baseada nos exemplos de https://learnopengl.com
//
#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>

#include <iostream>
#include <string>
#include <random>

// #define DEPTH_DEBUG
#define DEPTH_SHADER
#define ACUM_SHADER
#define FINAL_SHADER
#define LIGHT_SHADER

#define _APP "Forward Plus v0.1 - Leonardo Thomas"

using namespace std;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);
void setupLights();
void clearLightsStorageBuffers();

// confiurações da tela
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const float NEAR_PLANE = 0.1f;
const float FAR_PLANE = 100.0f;

const glm::vec3 MIN_COORDS = glm::vec3(-2.0, -2.0, -2.0);
const glm::vec3 MAX_COORDS = glm::vec3(2.0, 2.0, 2.0);

// configurações da câmera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// cálculo fps
float deltaTime = 0.0f;
float lastFrame = 0.0f;

//Buffers para armazenar dados sobre iluminação
GLuint lightBuffer = 0;
GLuint visibleLightIndicesBuffer = 0;
GLuint visibleLightCountBuffer = 0;


// Estruturas para armazenar dados da iluminação
struct LuzPontual {
	glm::vec3 position;
	glm::float32 constant;
	glm::float32 linear;
	glm::float32 quadratic;

	glm::vec3 ambient;
	glm::vec3 difuse;
	glm::vec3 specular;
};

struct LuzIndex {
	int index;
};

// luzes
const int MAX_LUZES = 10;

LuzPontual luzesPontuais[MAX_LUZES];

// modelo do cubo
// ------------------------------------------------------------------
float vertices[] = {
	// positions          // normals           // texture coords
	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
	0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
	0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
	0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
	-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
	0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
	0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
	0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
	-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
	-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

	0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
	0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
	0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
	0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
	0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
	0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
	0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
	0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
	0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
	0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
	-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
};

int main()
{
	// inicialização de glfw
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

	// criação da janela
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, _APP, NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}


	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// criação dos shaders
	// ------------------------------------
#if defined(DEPTH_SHADER)
	Shader depthShader("shaders/depth.vs", "shaders/depth.fs");
#endif
	Shader depthDebuggerShader("shaders/depth.debug.vs", "shaders/depth.debug.fs");
	Shader lightAcumShader("shaders/light.acum.vs", "shaders/light.acum.fs");
	Shader finalShader("shaders/final.shading.vs", "shaders/final.shading.fs");
	Shader lampShader("shaders/luzes.vs", "shaders/luzes.fs");

	//Buffer do depth para o passo 1 em forma de textura
	GLuint depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);

	GLuint depthMap;
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
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

	// Buffer para infos das luzes
	// Geração dos buffers de trabalho para os shaders
	glGenBuffers(1, &lightBuffer);
	glGenBuffers(1, &visibleLightIndicesBuffer);
	glGenBuffers(1, &visibleLightCountBuffer);

	// Bind para buffer de luzes
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_LUZES * sizeof(LuzPontual), 0, GL_STATIC_DRAW);

	// Bind para buffer de índices de luzes visíveis em cada pixel de tela
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, visibleLightIndicesBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, SCR_WIDTH * SCR_HEIGHT * sizeof(LuzIndex) * MAX_LUZES, 0, GL_STATIC_DRAW);

	// Bind para buffer de contadores para índices de luzes visíveis
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, visibleLightCountBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, SCR_WIDTH * SCR_HEIGHT * sizeof(LuzIndex), 0, GL_STATIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// VBO e VAO do cubo
	unsigned int VBO, cubeVAO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindVertexArray(cubeVAO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	// configura VBO e VAO das luzes
	unsigned int lightVAO;
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// note that we update the lamp's position attribute's stride to reflect the updated buffer data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//Iniciar luzes
	setupLights();


	// carrega texturas
	// -----------------------------------------------------------------------------
	unsigned int diffuseMap = loadTexture("textures/container2.png");
	unsigned int specularMap = loadTexture("textures/container2_specular.png");

	// variáveis uniform dos shaders
	// --------------------
	finalShader.use();
	finalShader.setInt("material.diffuse", 0);
	finalShader.setInt("material.specular", 1);

	lightAcumShader.use();
	lightAcumShader.setInt("material.diffuse", 0);
	// loop principal
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// fps
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// navegação com mouse e teclado
		// -----
		processInput(window);

		// transformações para posicionamento do modelo
		// -----
		glm::mat4 model;
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, NEAR_PLANE, FAR_PLANE);


		// render
		// ------
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Iniciar arrays de índices de luzes limpos
		clearLightsStorageBuffers();

#if defined(DEPTH_DEBUG)

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		depthDebuggerShader.use();
		depthDebuggerShader.setFloat("near", NEAR_PLANE);
		depthDebuggerShader.setFloat("far", FAR_PLANE);
		depthDebuggerShader.setMat4("model", model);
		depthDebuggerShader.setMat4("view", view);
		depthDebuggerShader.setMat4("projection", projection);

		// desenha cubo
		glBindVertexArray(cubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
#else

	#if defined(DEPTH_SHADER) 
		// Passo 1: Mapeia o depth da cena
		depthShader.use();
		depthShader.setMat4("model", model);
		depthShader.setMat4("view", view);
		depthShader.setMat4("projection", projection);
		// Bind no depth buffer e desenha nele
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		// desenha cubo 
		glBindVertexArray(cubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

	#endif // 

	#if defined(ACUM_SHADER) 
		//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Opengl 4.2+

		// Passo 2: Armazena pontos da cena que tiveram incidencia de luz
		lightAcumShader.use();
		lightAcumShader.setVec3("viewPos", camera.Position);
		lightAcumShader.setMat4("model", model);
		lightAcumShader.setMat4("view", view);
		lightAcumShader.setMat4("projection", projection);

		lightAcumShader.setInt("scr_height", SCR_HEIGHT);
		lightAcumShader.setInt("max_luzes", MAX_LUZES);

		// bind textura difusa
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffuseMap);


		// Bind do depth buffer no compute shader, posição 4 (demais posições já estão sendo usadas pelos objetos da cena)
		glActiveTexture(GL_TEXTURE4);
		glUniform1i(glGetUniformLocation(lightAcumShader.ID, "depthMap"), 4);
		glBindTexture(GL_TEXTURE_2D, depthMap);

		// Bind do storage buffer no shader para armazenar os índices das luzes
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, visibleLightIndicesBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, visibleLightCountBuffer);

		// cubo
		glBindVertexArray(cubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		// Unbind no depth buffer
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, 0);

	#endif // 

	#if defined(FINAL_SHADER) 
		finalShader.use();
		finalShader.setVec3("viewPos", camera.Position);

		// material properties
		finalShader.setFloat("material.shininess", 64.0f);

		// criação das matrizes MVP
		finalShader.setMat4("model", model);
		finalShader.setMat4("view", view);
		finalShader.setMat4("projection", projection);
		finalShader.setInt("scr_height", SCR_HEIGHT);

		// Bind do storage buffer no shader para armazenar os índices das luzes
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, visibleLightIndicesBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, visibleLightCountBuffer);

		// bind textura difusa
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffuseMap);
		// bind textura specular
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, specularMap);

		// desenha cubo
		glBindVertexArray(cubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

	#endif // 

	#if defined(LIGHT_SHADER)
		// desenhas as luzes
		// --------------------
		// seta uniforms
		lampShader.use();
		lampShader.setMat4("projection", projection);
		lampShader.setMat4("view", view);

		glBindVertexArray(lightVAO);


		glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightBuffer);
		LuzPontual *luzesPontuais = (LuzPontual*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);

		for (int i = 0; i < MAX_LUZES; i++) {
			model = glm::mat4();
			model = glm::translate(model, luzesPontuais[i].position);	// posiciona
			model = glm::scale(model, glm::vec3(0.2f)); // escala	
			lampShader.setMat4("model", model);			// passa uniform 
			
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}

		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	#endif // 

#endif
		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &lightVAO);
	glDeleteBuffers(1, &VBO);

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// inicia as luzes
// ---------------------------------------------------------------------------------------------------------
void setupLights() {
	//Escreve no buffer de luzes
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightBuffer);
	LuzPontual *luzesPontuais = (LuzPontual*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);

	random_device rd;
	mt19937 gen(rd());
	uniform_real_distribution<> dis(0, 1);


	for (int i = 0; i < MAX_LUZES; i++) {
		for (int j = 0; j < 3; j++) {
			float max = MAX_COORDS[j];
			float min = MIN_COORDS[j];

			float randomValue = (GLfloat)dis(gen) * (max - min) + min;//(GLfloat)dis(gen) * (max - min + 1) + min;
			luzesPontuais[i].position[j] = randomValue;
		}
		luzesPontuais[i].ambient = glm::vec3(0.05f, 0.05f, 0.05f);
		luzesPontuais[i].difuse = glm::vec3(0.8f, 0.8f, 0.8f);
		luzesPontuais[i].specular = glm::vec3(1.0f, 1.0f, 1.0f);
		luzesPontuais[i].constant = 1.0f;
		luzesPontuais[i].linear = 0.09;
		luzesPontuais[i].quadratic = 0.032;
	}

	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}


// Para ser chamado na inicialização e também no loop principal
void clearLightsStorageBuffers() {
	//Iniciar arrays de índices de luzes
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, visibleLightIndicesBuffer);
	LuzIndex *indicesLuzes = (LuzIndex*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);

	for (int i = 0; i < SCR_WIDTH; i++) {
		for (int j = 0; j < SCR_HEIGHT; j++) {
			for (int k = 0; k < MAX_LUZES; k++) {
				indicesLuzes[i * SCR_HEIGHT * MAX_LUZES + j * MAX_LUZES + k].index = 0;
			}
		}
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, visibleLightCountBuffer);
	LuzIndex *contIndicesLuzes = (LuzIndex*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);

	for (int i = 0; i < SCR_WIDTH; i++) {
		for (int j = 0; j < SCR_HEIGHT; j++) {
			contIndicesLuzes[i * SCR_HEIGHT + j].index = 0;
		}
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

}
// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}