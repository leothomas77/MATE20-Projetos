#pragma once

#include "model.h"
#include "shader.h"
#include "camera.h"

#include <GL\glew.h>
#include <GLFW\glfw3.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>

#define GLM_FORCE_RADIANS
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>

using namespace std;

//#define DEPTH_DEBUG
//#define LIGHT_DEBUG

// Constantes
const int WIDTH  = 800;
const int HEIGHT = 600;
const glm::ivec2 SCREEN_SIZE(WIDTH, HEIGHT);
const int TILE_SIZE = 16; //
const unsigned int NUM_LIGHTS = 1024;
const float LIGHT_RADIUS = 30.0f;
const float NEAR_PLANE = 0.1f;
const float FAR_PLANE = 300.0f;
const int MAX_LIGHTS = 1024;
const float LIGHT_STEP = -4.5f;

// Correção HDR - high dinamic range
const float exposure = 1.0f;

// Constantes das luzes
const glm::vec3 LIGHT_MIN_BOUNDS = glm::vec3(-135.0f, -20.0f, -60.0f);
const glm::vec3 LIGHT_MAX_BOUNDS = glm::vec3(135.0f, 170.0f, 60.0f);

GLFWwindow* gWindow;

// Área para desenhar o quadro
GLuint quadVAO = 0;
GLuint quadVBO;

// Variáveis de navegação na cena
bool keys[1024];
bool keysPressed[1024];
bool firstMouse = true;
GLfloat lastX = 400.0f, lastY = 300.0f;
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

//Buffers para armazenar dados sobre iluminação
GLuint lightBuffer = 0;
GLuint visibleLightIndicesBuffer = 0;

// Estruturas para armazenar dados da iluminação
struct PointLight {
	glm::vec4 color;
	glm::vec4 position;
	glm::vec4 paddingAndRadius;
};

struct VisibleIndex {
	int index;
};

// Dimensões X e Y para o compute shader
GLuint workGroupsX = 0;
GLuint workGroupsY = 0;

// Camera
Camera camera(glm::vec3(-40.0f, 10.0f, 0.0f));

// Criação da janela e inicialização do GLFW
void InitGLFW(int argc, char* argv[]);

// Inicialização dos buffers e da cena
void InitScene();

// Retorna randomicamente posicionamento das luzes
glm::vec3 RandomPosition(uniform_real_distribution<> dis, mt19937 gen);

// Inicializa as luzes com posições e cores randômicas
void SetupLights();

// Atualiza posição das luzes
void UpdateLights();

// Controle de navegação de mouse e teclado
void Movement();
static void KeyCallback(GLFWwindow *window, int key, int scanCode, int action, int mods);
static void MouseCallback(GLFWwindow *window, double x, double y);

// Based on function from LearnOpenGL: http://www.learnopengl.com
// Draw a 1 x 1 quad in NDC. We use it to render framebuffer color targets and post-processing effects
void DrawQuad();

int main(int argc, char **argv);
