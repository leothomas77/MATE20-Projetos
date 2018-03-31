#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void criarABuffers();
void criarBufferBasico();


// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

//ABuffer 
GLuint textura = 0;
GLuint texturaABuffer = 0;
GLuint contadorTexturaABuffer = 0;
GLuint depthBuffer = 0;
const int TAM_ABUFFER = 16;

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Transparencia Adaptativa - Leonardo Thomas", NULL, NULL);
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

	// build and compile shaders
	// -------------------------
	Shader shaderLimpar = Shader("shaders/basico.vert", "shaders/limpar.frag");
	Shader shaderPhong = Shader("shaders/phong.vert", "shaders/phong.frag");
	Shader shaderExibir = Shader("shaders/basico.vert", "shaders/exibir.frag");

	// load models
	// -----------
	Model modelLoaded("models/bunny.obj");

	criarABuffers();
	//criarBufferBasico();

	// draw in wireframe
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		// -----
		processInput(window);

		// render
		// ------
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);



		// view/projection transformations
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();

		// render the loaded model
		glm::mat4 model;
		model = glm::translate(model, glm::vec3(0.0f, -1.75f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));	// it's a bit too big for our scene, so scale it down
		
		//Parte 1. Limpa os buffers
		shaderLimpar.use();

		shaderLimpar.setInt("mapaABuffer", 0);
		shaderLimpar.setInt("contadorABuffer", 1);
		shaderLimpar.setInt("depthBuffer", 2);

		modelLoaded.Draw(shaderLimpar);

		//Parte 2. 
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Opengl 4.2+

		shaderPhong.use();
		shaderPhong.setInt("mapaABuffer", 0);
		shaderPhong.setInt("contadorABuffer", 1);
		shaderPhong.setInt("depthBuffer", 2);

		shaderPhong.setMat4("projection", projection);
		shaderPhong.setMat4("view", view);
		shaderPhong.setMat4("model", model);

		modelLoaded.Draw(shaderPhong);
		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
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

void criarABuffers() {
	//Textura 3D para armazenar os pixels dos fragmentos
	if (!texturaABuffer)
		glGenTextures(1, &texturaABuffer);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, texturaABuffer);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, SCR_WIDTH, SCR_HEIGHT, TAM_ABUFFER, 0, GL_RGBA, GL_FLOAT, 0);
	glBindImageTexture(0, texturaABuffer, 0, true, 0, GL_READ_WRITE, GL_RGBA32F);

	//check_gl_error();

	//Textura 2D para armazenar a quantidade de fragmentos
	if (!contadorTexturaABuffer)
		glGenTextures(1, &contadorTexturaABuffer);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, contadorTexturaABuffer);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, 0);
	glBindImageTexture(1, contadorTexturaABuffer, 0, false, 0, GL_READ_WRITE, GL_R32UI);

	//check_gl_error();

	//Um buffer para armazenar o valor do depth
	if (!depthBuffer)
		glGenTextures(1, &depthBuffer);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D_ARRAY, depthBuffer);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R32F, SCR_WIDTH, SCR_HEIGHT, TAM_ABUFFER, 0, GL_RED, GL_FLOAT, 0);
	glBindImageTexture(2, depthBuffer, 0, false, 0, GL_READ_WRITE, GL_R32F);

	//check_gl_error();
}

/*
void criarBufferBasico() {

	glGenBuffers(1, &textura);
	glBindBuffer(GL_ARRAY_BUFFER, textura);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVArray), quadVArray, GL_STATIC_DRAW);

	check_gl_error();


}*/