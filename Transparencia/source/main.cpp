#include <iostream>
#include <cfloat>
#include <vector>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/vec3.hpp> 
#include <glm/vec4.hpp> 
#include <glm/mat4x4.hpp> 
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp> 

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include "initShaders.h"
#include "glError.h"

using namespace std;
/// ***********************************************************************
/// **
/// ***********************************************************************

#ifndef BUFFER_OFFSET 
#define BUFFER_OFFSET(i) ((char *)NULL + (i))
#endif
#ifndef MIN
#define MIN(x,y) (x<y?x:y)
#endif
#ifndef MAX
#define MAX(x,y) (y>x?y:x)
#endif

#define _APP "Transparencia Adaptativa v1.0 - Leonardo Thomas"

using namespace std;
//using namespace glm;

GLuint 	shaderAmbient,
shaderGouraud,
shaderPhong,
shaderLimpar,
shaderRender,
shaderExibir,
shader;
GLuint 	axisVBO[3];
//GLuint 	meshVBO[3];
GLuint bufferVertices, bufferNormais, bufferCores;
GLuint 	meshSize;

GLuint texturaMapa;

//ABuffer 
GLuint textura = 0;
GLuint texturaABuffer = 0;
GLuint contadorTexturaABuffer = 0;
GLuint depthBuffer = 0;

//Calculo do FPS
unsigned contFPS = 0;
float lastFPS = 0.0, intervaloFPS = 0.0;

const int TAM_ABUFFER = 16;
const GLfloat quadVArray[] = {
	-1.0f, -1.0f, 0.0f, 1.0f,
	1.0f, -1.0f, 0.0f, 1.0f,
	-1.0f, 1.0f, 0.0f, 1.0f,
	1.0f, -1.0f, 0.0f, 1.0f,
	1.0f, 1.0f, 0.0f, 1.0f,
	-1.0f, 1.0f, 0.0f, 1.0f
};


double  last;

vector<GLfloat> vboVertices;
vector<GLfloat> vboNormals;
vector<GLfloat> vboColors;

int winWidth = 600,
winHeight = 600;

float 	angleX = 0.0f,
angleY = 0.0f,
angleZ = 0.0f;

const double FAR = 50.0f;
const double NEAR = 0.01f;

bool comAT = false;
bool debug = false;

/* the global Assimp scene object */
const aiScene* scene = NULL;
GLuint scene_list = 0;
aiVector3D scene_min, scene_max, scene_center;

void get_bounding_box_for_node(const struct aiNode* nd,
	aiVector3D* min,
	aiVector3D* max,
	aiMatrix4x4* trafo
);
int traverseScene(const aiScene *sc, const aiNode* nd, vector<GLfloat> &vboColors, vector<GLfloat> &vboVertices);

static void loadMesh(char* filename);

void createVBOs(const aiScene *sc);

void showFPS(float currentFrame, float &lastFrameFPS, float &deltaTimeFPS, unsigned int &fps, GLFWwindow* window) {

	char titulo[100];
	titulo[99] = '\0';

	if (deltaTimeFPS > 1.0f) {
		lastFrameFPS = currentFrame;
		snprintf(titulo, 99, "%s - [FPS: %3.0u]", _APP, fps);
		fps = 0;
		deltaTimeFPS = 0;
		glfwSetWindowTitle(window, titulo);
	}
}

/* ---------------------------------------------------------------------------- */
void get_bounding_box_for_node(const struct aiNode* nd,
	aiVector3D* min,
	aiVector3D* max,
	aiMatrix4x4* trafo
) {
	aiMatrix4x4 prev;
	unsigned int n = 0, t;

	prev = *trafo;
	aiMultiplyMatrix4(trafo, &nd->mTransformation);

	for (; n < nd->mNumMeshes; ++n) {
		const aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];
		for (t = 0; t < mesh->mNumVertices; ++t) {

			aiVector3D tmp = mesh->mVertices[t];
			aiTransformVecByMatrix4(&tmp, trafo);

			min->x = MIN(min->x, tmp.x);
			min->y = MIN(min->y, tmp.y);
			min->z = MIN(min->z, tmp.z);

			max->x = MAX(max->x, tmp.x);
			max->y = MAX(max->y, tmp.y);
			max->z = MAX(max->z, tmp.z);
		}
	}

	for (n = 0; n < nd->mNumChildren; ++n) {
		get_bounding_box_for_node(nd->mChildren[n], min, max, trafo);
	}
	*trafo = prev;
}


int traverseScene(const aiScene *sc, const aiNode* nd, vector<GLfloat> &vboColors, vector<GLfloat> &vboVertices) {

	int totVertices = 0;

	/* draw all meshes assigned to this node */
	for (unsigned int n = 0; n < nd->mNumMeshes; ++n) {
		const aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];

		for (unsigned int t = 0; t < mesh->mNumFaces; ++t) {
			const aiFace* face = &mesh->mFaces[t];

			for (unsigned int i = 0; i < face->mNumIndices; i++) {
				int index = face->mIndices[i];
				//				if(mesh->mColors[0] != NULL) {
				vboColors.push_back(0.5);
				vboColors.push_back(0.5);
				vboColors.push_back(1.0);
				vboColors.push_back(1.0);
				//					}
				if (mesh->mNormals != NULL) {
					vboNormals.push_back(mesh->mNormals[index].x);
					vboNormals.push_back(mesh->mNormals[index].y);
					vboNormals.push_back(mesh->mNormals[index].z);
				}
				vboVertices.push_back(mesh->mVertices[index].x);
				vboVertices.push_back(mesh->mVertices[index].y);
				vboVertices.push_back(mesh->mVertices[index].z);
				totVertices++;
			}
		}
	}

	for (unsigned int n = 0; n < nd->mNumChildren; ++n) {
		totVertices += traverseScene(sc, nd->mChildren[n], vboColors, vboVertices);
	}
	return totVertices;
}

static void loadMesh(char* filename) {

	scene = aiImportFile(filename, aiProcessPreset_TargetRealtime_MaxQuality);

	if (!scene) {
		cout << "## ERROR loading mesh" << endl;
		exit(-1);
	}

	aiMatrix4x4 trafo;
	aiIdentityMatrix4(&trafo);

	aiVector3D min, max;

	scene_min.x = scene_min.y = scene_min.z = FLT_MAX;
	scene_max.x = scene_max.y = scene_max.z = -FLT_MAX;

	get_bounding_box_for_node(scene->mRootNode, &scene_min, &scene_max, &trafo);

	scene_center.x = (scene_min.x + scene_max.x) / 2.0f;
	scene_center.y = (scene_min.y + scene_max.y) / 2.0f;
	scene_center.z = (scene_min.z + scene_max.z) / 2.0f;

	scene_min.x *= 1.2;
	scene_min.y *= 1.2;
	scene_min.z *= 1.2;
	scene_max.x *= 1.2;
	scene_max.y *= 1.2;
	scene_max.z *= 1.2;

	if (scene_list == 0)
		createVBOs(scene);

	cout << "Bounding Box: " << " ( " << scene_min.x << " , " << scene_min.y << " , " << scene_min.z << " ) - ( "
		<< scene_max.x << " , " << scene_max.y << " , " << scene_max.z << " )" << endl;
	cout << "Bounding Box: " << " ( " << scene_center.x << " , " << scene_center.y << " , " << scene_center.z << " )" << endl;
}

void createVBOs(const aiScene *sc) {

	int totVertices = 0;
	cout << "Scene:	 	#Meshes 	= " << sc->mNumMeshes << endl;
	cout << "			#Textures	= " << sc->mNumTextures << endl;

	totVertices = traverseScene(sc, sc->mRootNode, vboColors, vboVertices);

	cout << "			#Vertices	= " << totVertices << endl;
	cout << "			#vboVertices= " << vboVertices.size() << endl;
	cout << "			#vboColors= " << vboColors.size() << endl;
	cout << "			#vboNormals= " << vboNormals.size() << endl;

	glGenBuffers(1, &bufferVertices);
	glBindBuffer(GL_ARRAY_BUFFER, bufferVertices);
	glBufferData(GL_ARRAY_BUFFER, vboVertices.size() * sizeof(float), vboVertices.data(), GL_STATIC_DRAW);

	if (vboNormals.size() > 0) {
		glGenBuffers(1, &bufferNormais);
		glBindBuffer(GL_ARRAY_BUFFER, bufferNormais);
		glBufferData(GL_ARRAY_BUFFER, vboNormals.size() * sizeof(float), vboNormals.data(), GL_STATIC_DRAW);
	}

	meshSize = vboVertices.size() / 3;
	cout << "			#meshSize= " << meshSize << endl;
}

void criarABuffers() {
	//Textura 3D para armazenar os pixels dos fragmentos
	if(!texturaABuffer)
		glGenTextures(1, &texturaABuffer);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, texturaABuffer);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, winWidth, winHeight, TAM_ABUFFER, 0, GL_RGBA, GL_FLOAT, 0);
	glBindImageTexture(0, texturaABuffer, 0, true, 0, GL_READ_WRITE, GL_RGBA32F);
	
	check_gl_error();

	//Textura 2D para armazenar a quantidade de fragmentos
	if (!contadorTexturaABuffer)
		glGenTextures(1, &contadorTexturaABuffer);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, contadorTexturaABuffer);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, winWidth, winHeight, 0, GL_RED, GL_FLOAT, 0);
	glBindImageTexture(1, contadorTexturaABuffer, 0, false, 0, GL_READ_WRITE, GL_R32UI);

	check_gl_error();

	//Um buffer para armazenar o valor do depth
	if (!depthBuffer)
		glGenTextures(1, &depthBuffer);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D_ARRAY, depthBuffer);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R32F, winWidth, winHeight, TAM_ABUFFER, 0, GL_RED, GL_FLOAT, 0);
	glBindImageTexture(2, depthBuffer, 0, false, 0, GL_READ_WRITE, GL_R32F);

	check_gl_error();
}

void criarBufferBasico() {

	glGenBuffers(1, &textura);
	glBindBuffer(GL_ARRAY_BUFFER, textura);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVArray), quadVArray, GL_STATIC_DRAW);

	check_gl_error();


}

void drawBasico(GLint shader) {

	int attrV;
	attrV = glGetAttribLocation(shader, "aPosition");
	glEnableVertexAttribArray(attrV);

	glBindBuffer(GL_ARRAY_BUFFER, textura);
	glVertexAttribPointer(attrV, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, 0);
	glDrawArrays(GL_TRIANGLES, 0, 24);

	check_gl_error();

}

void drawModel(GLint shader, glm::mat4 MVP, glm::mat4 normalMat, glm::mat4 ModelMat) {

	int attrV, attrN;

	glBindBuffer(GL_ARRAY_BUFFER, bufferVertices);
	attrV = glGetAttribLocation(shader, "aPosition");
	glVertexAttribPointer(attrV, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attrV);

	glBindBuffer(GL_ARRAY_BUFFER, bufferNormais);
	attrN = glGetAttribLocation(shader, "aNormal");
	glVertexAttribPointer(attrN, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attrN);

	glDrawArrays(GL_TRIANGLES, 0, meshSize);

	glDisableVertexAttribArray(attrV);
	glDisableVertexAttribArray(attrN);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	check_gl_error();

}

void display(void) {

	angleY += 0.0015;

	float Max = MAX(scene_max.x, MAX(scene_max.y, scene_max.z));

	glm::vec3 lightPos = glm::vec3(Max, Max, 0.0);
	glm::vec3 camPos = glm::vec3(1.5f*Max, 1.5f*Max, 1.5f*Max);
	glm::vec3 lookAt = glm::vec3(scene_center.x, scene_center.y, scene_center.z);
	glm::vec3 up = glm::vec3(0.0, 1.0, 0.0);

	glm::mat4 ViewMat = glm::lookAt(camPos, lookAt, up);

	glm::mat4 ProjMat = glm::perspective(70.0, 1.0, NEAR, FAR);
	glm::mat4 ModelMat = glm::mat4(1.0);

	ModelMat = glm::rotate(ModelMat, angleX, glm::vec3(1.0, 0.0, 0.0));
	ModelMat = glm::rotate(ModelMat, angleY, glm::vec3(0.0, 1.0, 0.0));
	ModelMat = glm::rotate(ModelMat, angleZ, glm::vec3(0.0, 0.0, 1.0));

	glm::mat4 MVP = ProjMat * ViewMat * ModelMat;

	glm::mat4 normalMat = glm::transpose(glm::inverse(ModelMat));

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//Passo 1: Limpa os buffers
	glUseProgram(shaderLimpar);
	glProgramUniform1i(shaderLimpar, glGetUniformLocation(shaderLimpar, "mapaABuffer"), 0);
	glProgramUniform1i(shaderLimpar, glGetUniformLocation(shaderLimpar, "contadorABuffer"), 1);
	glProgramUniform1i(shaderLimpar, glGetUniformLocation(shaderLimpar, "depthBuffer"), 2);
	check_gl_error();
	drawBasico(shaderLimpar);

	//Passo 2: Preenche arrays com contadores, depths e cor de fargmentos. Shader phong
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Opengl 4.2+

	glUseProgram(shaderPhong);
	glProgramUniform1i(shaderPhong, glGetUniformLocation(shaderPhong, "mapaABuffer"), 0);
	glProgramUniform1i(shaderPhong, glGetUniformLocation(shaderPhong, "contadorABuffer"), 1);
	glProgramUniform1i(shaderPhong, glGetUniformLocation(shaderPhong, "depthBuffer"), 2);
	check_gl_error();

	glUniformMatrix4fv(glGetUniformLocation(shaderPhong, "uMVP"), 1, GL_FALSE, glm::value_ptr(MVP));
	glUniformMatrix4fv(glGetUniformLocation(shaderPhong, "uN"), 1, GL_FALSE, glm::value_ptr(normalMat));
	glUniformMatrix4fv(glGetUniformLocation(shaderPhong, "uM"), 1, GL_FALSE, glm::value_ptr(ModelMat));
	glUniform3fv(glGetUniformLocation(shaderPhong, "uLPos"), 1, glm::value_ptr(lightPos));
	glUniform3fv(glGetUniformLocation(shaderPhong, "uCamPos"), 1, glm::value_ptr(camPos));
	//glUniform1f(glGetUniformLocation(shaderPhong, "near"), NEAR);
	//glUniform1f(glGetUniformLocation(shaderPhong, "far"), FAR);
	check_gl_error();

	drawModel(shaderPhong, MVP, normalMat, ModelMat);

	//Passo 3: Exibir resultado
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Opengl 4.2+
	
	glUseProgram(shaderExibir);
	glProgramUniform1i(shaderExibir, glGetUniformLocation(shaderExibir, "mapaABuffer"), 0);
	glProgramUniform1i(shaderExibir, glGetUniformLocation(shaderExibir, "contadorABuffer"), 1);
	glProgramUniform1i(shaderExibir, glGetUniformLocation(shaderExibir, "depthBuffer"), 2);
	glProgramUniform1i(shaderExibir, glGetUniformLocation(shaderExibir, "comAT"), comAT);
	glProgramUniform1i(shaderExibir, glGetUniformLocation(shaderExibir, "debug"), debug);

	check_gl_error();

	drawBasico(shaderExibir);

}

void initGL(GLFWwindow* window) {

	if (glewInit()) {
		cout << "Unable to initialize GLEW ... exiting" << endl;
		exit(EXIT_FAILURE);
	}

	cout << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << endl;

	cout << "Opengl Version: " << glGetString(GL_VERSION) << endl;
	cout << "Opengl Vendor : " << glGetString(GL_VENDOR) << endl;
	cout << "Opengl Render : " << glGetString(GL_RENDERER) << endl;
	cout << "Opengl Shading Language Version : " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;


	glPointSize(3.0);
//	glEnable(GL_DEPTH_TEST);
//	glEnable(GL_NORMALIZE);

	glDisable(GL_CULL_FACE); //Vou precisar de todas as facess, tanto as faces de frente quanto as faces de trás
	glDisable(GL_DEPTH_TEST);//Vou controlar o depth test, pois não há apenas objetos opacos na tela
	glDisable(GL_STENCIL_TEST);//
	glDisable(GL_BLEND);// Não vou usar a função built in de blending

}

void initShaders(void) {

	// Load shaders and use the resulting shader program
	shaderLimpar = InitShader("shaders/basico.vs", "shaders/limpar.fs");
	shaderPhong = InitShader("shaders/phong.vs", "shaders/phong.fs");
	shaderExibir = InitShader("shaders/basico.vs", "shaders/exibir.fs");

}

static void error_callback(int error, const char* description) {
	cout << "Error: " << description << endl;
}

static void window_size_callback(GLFWwindow* window, int width, int height) {

	glViewport(0, 0, width, height);
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	if (action == GLFW_PRESS)
		switch (key) {
		case GLFW_KEY_ESCAPE: 	glfwSetWindowShouldClose(window, true);
			break;
		case '.': 	glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			break;

		case '-': 	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;

		case 'F':
		case 'f': 	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			break;
		case 'P':
		case 'p': 	shader = shaderPhong;
			break;
		case 'T':
		case 't': 	comAT = !comAT;
			break;
		case 'D':
		case 'd': 	debug = !debug;
			break;
		}

}

static GLFWwindow* initGLFW(char* nameWin, int w, int h) {

	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	GLFWwindow* window = glfwCreateWindow(w, h, nameWin, NULL, NULL);
	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetWindowSizeCallback(window, window_size_callback);

	glfwSetKeyCallback(window, key_callback);

	glfwMakeContextCurrent(window);

	glfwSwapInterval(1);

	return (window);
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */
static void GLFW_MainLoop(GLFWwindow* window) {

	while (!glfwWindowShouldClose(window)) {

		double now = glfwGetTime();
		double ellapsed = now - last;
		float intervaloFPS = now - lastFPS;

		if (ellapsed > 1.0f / 60.0f) {
			last = now;
			display();
			glfwSwapBuffers(window);

		}
		//contFPS++;
		
		//showFPS(now, lastFPS, intervaloFPS, contFPS, window);

		glfwPollEvents();
	}
}

static void initASSIMP() {

	struct aiLogStream stream;

	/* get a handle to the predefined STDOUT log stream and attach
	it to the logging system. It remains active for all further
	calls to aiImportFile(Ex) and aiApplyPostProcessing. */
	stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT, NULL);
	aiAttachLogStream(&stream);
}

int main(int argc, char *argv[]) {

	GLFWwindow* window;

	char meshFilename[] = "models/bunny.obj";

	window = initGLFW(argv[0], winWidth, winHeight);

	initASSIMP();
	initGL(window);
	initShaders();

	if (argc > 1)
		loadMesh(argv[1]);
	else
		loadMesh(meshFilename);

	criarABuffers();
	criarBufferBasico();

	GLFW_MainLoop(window);

	glfwDestroyWindow(window);
	glfwTerminate();

	aiReleaseImport(scene);
	aiDetachAllLogStreams();

	exit(EXIT_SUCCESS);
}
