#include <iostream>
#include <cfloat>
#include <vector>
#include <random>


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

#ifndef BUFFER_OFFSET 
#define BUFFER_OFFSET(i) ((char *)NULL + (i))
#endif


#ifndef MIN
#define MIN(x,y) (x<y?x:y)
#endif
#ifndef MAX
#define MAX(x,y) (y>x?y:x)
#endif

#define QUANT_LUZES 100

const glm::vec3 POSICAO_LUZ_MINIMA = glm::vec3(-135.0f, -20.0f, -60.0f);
const glm::vec3 POSICAO_LUZ_MAXIMA = glm::vec3(135.0f, 170.0f, 60.0f);

struct PontoDeLuz {
	glm::vec4 cor;
	glm::vec4 posicao;
	glm::vec4 raio;
};

struct ContadorLuzesVisiveis {
	int quant;
};


using namespace std;

void initShaders(void);

void iniciaLuzes();

static void initASSIMP();

static const aiScene* loadMesh(char* filename, aiVector3D &scene_min, aiVector3D &scene_max, aiVector3D &scene_center);

void createVBOs(const aiScene *sc, vector<GLfloat> &vertices, vector<GLfloat> &cores, vector<GLfloat> &normais);

void get_bounding_box_for_node(
	const aiScene* scene,
	const struct aiNode* nd,
	aiVector3D* min,
	aiVector3D* max,
	aiMatrix4x4* trafo
);

int traverseScene(const aiScene *sc, const aiNode* nd, vector<GLfloat> &vertices, vector<GLfloat> &cores, vector<GLfloat> &normais);



GLuint 	shaderBasico, shaderDepth, shaderCull, shaderAcum, shaderRender;

GLuint bufferLuzes, bufferLuzesVisiveis;

GLuint quadVAO, quadVBO;

//Modelo
GLuint 	meshVBO[3];
GLuint 	axisVBO[3];

GLuint	meshSize;
vector<GLfloat> vboVertices;
vector<GLfloat> vboNormals;
vector<GLfloat> vboColors;

//ABuffer 
GLuint textura = 0;
GLuint texturaABuffer = 0;
GLuint contadorTexturaABuffer = 0;
GLuint depthBuffer = 0;
const int TAM_ABUFFER = 16;


GLuint depthMapFBO;
GLuint hdrFBO;
GLuint colorBuffer;
GLuint rboDepth;
GLuint depthMap;

const float exposure = 1.0f;

double  last;


glm::ivec2 tela = glm::ivec2(600, 600);

int winWidth = 600,
winHeight = 600;

float 	angleX = 0.0f,
angleY = 0.0f,
angleZ = 0.0f;


/* the global Assimp scene object */
const aiScene* scene = NULL;
//const aiScene* scenePlane = NULL;


GLuint scene_list = 0;
aiVector3D scene_min, scene_max, scene_center;

GLuint grupoX, grupoY;


void get_bounding_box_for_node(const aiScene* scene, const struct aiNode* nd,
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
		get_bounding_box_for_node(scene, nd->mChildren[n], min, max, trafo);
	}
	*trafo = prev;
}



void createVBOs(const aiScene *sc, vector<GLfloat> &vertices, vector<GLfloat> &cores, vector<GLfloat> &normais) {

	int totVertices = 0;
	cout << "Scene:	 	#Meshes 	= " << sc->mNumMeshes << endl;
	cout << "			#Textures	= " << sc->mNumTextures << endl;

	totVertices = traverseScene(sc, sc->mRootNode, vertices, cores, normais);

	cout << "			#Vertices	= " << totVertices << endl;
	cout << "			#vboVertices= " << vertices.size() << endl;
	cout << "			#vboColors= " << cores.size() << endl;
	cout << "			#vboNormals= " << normais.size() << endl;

	glGenBuffers(3, meshVBO);

	glBindBuffer(GL_ARRAY_BUFFER, meshVBO[0]);

	glBufferData(GL_ARRAY_BUFFER, vboVertices.size() * sizeof(float),
		vboVertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, meshVBO[1]);

	glBufferData(GL_ARRAY_BUFFER, vboColors.size() * sizeof(float),
		vboColors.data(), GL_STATIC_DRAW);

	if (vboNormals.size() > 0) {
		glBindBuffer(GL_ARRAY_BUFFER, meshVBO[2]);

		glBufferData(GL_ARRAY_BUFFER, vboNormals.size() * sizeof(float),
			vboNormals.data(), GL_STATIC_DRAW);
	}

	meshSize = vboVertices.size() / 3;
	cout << "			#meshSize= " << meshSize << endl;
}

int traverseScene(const aiScene *scene, const aiNode* nd, vector<GLfloat> &vertices, vector<GLfloat> &cores, vector<GLfloat> &normais) {

	int totVertices = 0;

	/* draw all meshes assigned to this node */
	for (unsigned int n = 0; n < nd->mNumMeshes; ++n) {
		const aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];

		bool hasColor = mesh->GetNumColorChannels();


		for (unsigned int t = 0; t < mesh->mNumFaces; ++t) {
			const aiFace* face = &mesh->mFaces[t];

			for (unsigned int i = 0; i < face->mNumIndices; i++) {
				int index = face->mIndices[i];
				if (!hasColor) {
					cores.push_back(0.5);
					cores.push_back(0.5);
					cores.push_back(1.0);
					cores.push_back(1.0);
				}
				else {
					cores.push_back(mesh->mColors[0][t].r);
					cores.push_back(mesh->mColors[0][t].r);
					cores.push_back(mesh->mColors[0][t].r);
					cores.push_back(mesh->mColors[0][t].r);
				}
				if (mesh->mNormals != NULL) {
					normais.push_back(mesh->mNormals[index].x);
					normais.push_back(mesh->mNormals[index].y);
					normais.push_back(mesh->mNormals[index].z);
				}
				vertices.push_back(mesh->mVertices[index].x);
				vertices.push_back(mesh->mVertices[index].y);
				vertices.push_back(mesh->mVertices[index].z);
				totVertices++;
			}
		}
	}

	for (unsigned int n = 0; n < nd->mNumChildren; ++n) {
		totVertices += traverseScene(scene, nd->mChildren[n], vertices, cores, normais);
	}
	return totVertices;
}

void criarBuffers() {

	grupoX = (winWidth + (winWidth % 16)) / 16;
	grupoY = (winHeight + (winHeight % 16)) / 16;
	size_t quantQuadros = grupoX * grupoY;

	glGenBuffers(1, &bufferLuzes);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferLuzes);
	glBufferData(GL_SHADER_STORAGE_BUFFER, QUANT_LUZES * sizeof(PontoDeLuz), 0, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &bufferLuzesVisiveis);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferLuzesVisiveis);
	glBufferData(GL_SHADER_STORAGE_BUFFER, quantQuadros * sizeof(ContadorLuzesVisiveis) * QUANT_LUZES, 0, GL_DYNAMIC_DRAW);

	iniciaLuzes();

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	
	
	glGenFramebuffers(1, &depthMapFBO);
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, winWidth, winHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
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

	glGenFramebuffers(1, &hdrFBO);

	glGenTextures(1, &colorBuffer);
	glBindTexture(GL_TEXTURE_2D, colorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, winWidth, winHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, winWidth, winHeight);

	glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

glm::vec3 rendomizarPosicao(uniform_real_distribution<> dis, mt19937 gen) {
	glm::vec3 position = glm::vec3(0.0);
	for (int i = 0; i < 3; i++) {
		float min = POSICAO_LUZ_MINIMA[i];
		float max = POSICAO_LUZ_MAXIMA[i];
		position[i] = (GLfloat)dis(gen) * (max - min) + min;
	}

	return position;
}

void iniciaLuzes() { //Pode ser feito um shader para iniciar
	if (bufferLuzes == 0)
		return;


}

void updateLuzes() {

	return;
}

void drawMesh(GLint shader) {

	int attrV, attrC, attrN;

	glBindBuffer(GL_ARRAY_BUFFER, meshVBO[0]);
	attrV = glGetAttribLocation(shader, "aPosition");
	glVertexAttribPointer(attrV, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attrV);

	glBindBuffer(GL_ARRAY_BUFFER, meshVBO[1]);
	attrC = glGetAttribLocation(shader, "aColor");
	glVertexAttribPointer(attrC, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attrC);

	glBindBuffer(GL_ARRAY_BUFFER, meshVBO[2]);
	attrN = glGetAttribLocation(shader, "aNormal");
	glVertexAttribPointer(attrN, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attrN);

	glDrawArrays(GL_TRIANGLES, 0, meshSize);

	glDisableVertexAttribArray(attrV);
	glDisableVertexAttribArray(attrC);
	glDisableVertexAttribArray(attrN);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void drawQuad() {
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

void createAxis() {

	GLfloat vertices[] = { 0.0, 0.0, 0.0,
		scene_max.x * 2, 0.0, 0.0,
		0.0, scene_max.y * 2, 0.0,
		0.0, 0.0, scene_max.z * 2
	};

	GLuint lines[] = { 0, 3,
		0, 2,
		0, 1
	};

	GLfloat colors[] = { 1.0, 1.0, 1.0, 1.0,
		1.0, 0.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		0.0, 0.0, 1.0, 1.0
	};



	glGenBuffers(3, axisVBO);

	glBindBuffer(GL_ARRAY_BUFFER, axisVBO[0]);

	glBufferData(GL_ARRAY_BUFFER, 4 * 3 * sizeof(float),
		vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, axisVBO[1]);

	glBufferData(GL_ARRAY_BUFFER, 4 * 4 * sizeof(float),
		colors, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, axisVBO[2]);

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * 2 * sizeof(unsigned int),
		lines, GL_STATIC_DRAW);

}

/// ***********************************************************************
/// **
/// ***********************************************************************

void drawAxis(GLuint shader) {

	int attrV, attrC;

	glBindBuffer(GL_ARRAY_BUFFER, axisVBO[0]);
	attrV = glGetAttribLocation(shader, "aPosition");
	glVertexAttribPointer(attrV, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attrV);

	glBindBuffer(GL_ARRAY_BUFFER, axisVBO[1]);
	attrC = glGetAttribLocation(shader, "aColor");
	glVertexAttribPointer(attrC, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attrC);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, axisVBO[2]);
	glDrawElements(GL_LINES, 6, GL_UNSIGNED_INT, BUFFER_OFFSET(0));

	glDisableVertexAttribArray(attrV);
	glDisableVertexAttribArray(attrC);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void display(void) {


	//le eventos

	//move camera

	//atualiza luzes

	angleY += 0.02;

	float Max = max(scene_max.x, max(scene_max.y, scene_max.z));

	glm::vec3 lightPos = glm::vec3(Max, Max, 0.0);
	glm::vec3 camPos = glm::vec3(1.5f*Max, 1.5f*Max, 1.5f*Max);
	glm::vec3 lookAt = glm::vec3(scene_center.x, scene_center.y, scene_center.z);
	glm::vec3 up = glm::vec3(0.0, 1.0, 0.0);

	glm::mat4 ViewMat = glm::lookAt(camPos, lookAt, up);

	glm::mat4 ProjMat = glm::perspective(70.0, 1.0, 0.01, 50.0);
	glm::mat4 ModelMat = glm::mat4(1.0);

	ModelMat = glm::rotate(ModelMat, angleX, glm::vec3(1.0, 0.0, 0.0));
	ModelMat = glm::rotate(ModelMat, angleY, glm::vec3(0.0, 1.0, 0.0));
	ModelMat = glm::rotate(ModelMat, angleZ, glm::vec3(0.0, 0.0, 1.0));


	glm::mat4 MVP = ProjMat * ViewMat * ModelMat;

	glm::mat4 normalMat = glm::transpose(glm::inverse(ModelMat));

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Preenche uniforms
	glUseProgram(shaderBasico);

	int loc = glGetUniformLocation(shaderBasico, "uMVP");
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(MVP));


	drawAxis(shaderBasico);

	//Passo 1 - Depth para criar o depth map
	glUseProgram(shaderDepth);
	glUniformMatrix4fv(glGetUniformLocation(shaderDepth, "model"), 1, GL_FALSE, glm::value_ptr(ModelMat));
	glUniformMatrix4fv(glGetUniformLocation(shaderDepth, "projection"), 1, GL_FALSE, glm::value_ptr(MVP));
	glUniformMatrix4fv(glGetUniformLocation(shaderDepth, "view"), 1, GL_FALSE, glm::value_ptr(ViewMat));

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	drawQuad();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//Barrier aqui
	//Passo 2 - descarte de luzes
	glUseProgram(shaderCull);
	glUniform1i(glGetUniformLocation(shaderCull, "lightCount"), QUANT_LUZES);
	glUniform2iv(glGetUniformLocation(shaderCull, "screenSize"), 1, &tela.x);

	glActiveTexture(GL_TEXTURE4);
	glUniform1i(glGetUniformLocation(shaderCull, "depthMap"), 4);
	glBindTexture(GL_TEXTURE_2D, depthMap);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, bufferLuzes);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, bufferLuzesVisiveis);

	glDispatchCompute(grupoX, grupoY, 1);

	//
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, 0);


	////Render final
	// We render the scene into the floating point HDR frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaderAcum);
	glUniformMatrix4fv(glGetUniformLocation(shaderAcum, "projection"), 1, GL_FALSE, glm::value_ptr(MVP));
	glUniformMatrix4fv(glGetUniformLocation(shaderAcum, "view"), 1, GL_FALSE, glm::value_ptr(ViewMat));
	glUniform3fv(glGetUniformLocation(shaderAcum, "viewPosition"), 1, &camPos[0]);

	

	//sponzaModel.Draw(lightAccumulationShader); //Parei aqui
	glActiveTexture(GL_TEXTURE1);
	glUniform1i(glGetUniformLocation(shaderAcum, (name + number).c_str()), i);
	glBindTexture(GL_TEXTURE_2D, this->textures[i].id);

	// Draw mesh
	glBindVertexArray(this->VAO);
	glDrawElements(GL_TRIANGLES, (GLsizei)this->indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	// Reset to defaults
	for (GLuint i = 0; i < this->textures.size(); i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	/*
	// Tonemap the HDR colors to the default framebuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Weirdly, moving this call drops performance into the floor
	glUseProgram(shaderRender);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colorBuffer);
	glUniform1f(glGetUniformLocation(hdrShader.Program, "exposure"), exposure);
	DrawQuad();
	
	*/

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);


}

void criaMesh() {
	// Create buffers and arrays
	glGenVertexArrays(1, &this->VAO);
	glGenBuffers(1, &this->VBO);
	glGenBuffers(1, &this->EBO);

	glBindVertexArray(this->VAO);
	// Load data into vertex buffers
	glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
	glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(Vertex), &this->vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(GLuint), &this->indices[0], GL_STATIC_DRAW);

	// Set the vertex attribute pointers
	// Positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);

	// Normals
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));

	// Texture Coords
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, textureCoordinates));

	// Tangent
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, tangent));

	// Bitangent
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, bitangent));

	glBindVertexArray(0);
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

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

	//Habilita/desabilita features
	glEnable(GL_DEPTH_TEST); 
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);

}

void initGL_Old(GLFWwindow* window) {

	glClearColor(0.0, 0.0, 0.0, 0.0);

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

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

void initShaders(void) {

	shaderBasico = InitShader("basicShader.vert", "basicShader.frag");

	shaderDepth = InitShader("depth.vert", "depth.frag");
	shaderCull = InitComputeShader("light_culling.comp");

	shaderAcum = InitShader("light_accumulation.vert", "light_accumulation.frag");
	/*	shaderRender = InitShader("hdr.vert", "hdr.frag");
*/
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

static void error_callback(int error, const char* description) {
	cout << "Error: " << description << endl;
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

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

		}

}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

static GLFWwindow* initGLFW_new(char* nameWin, int w, int h) {

	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	GLFWwindow* window = glfwCreateWindow(w, h, nameWin, NULL, NULL);
	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);

	glfwSetWindowSizeCallback(window, window_size_callback);

	glfwSetKeyCallback(window, key_callback);

	glfwMakeContextCurrent(window);

	glfwSwapInterval(1);

	return (window);
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

		if (ellapsed > 1.0f / 30.0f) {
			last = now;
			display();
			glfwSwapBuffers(window);
		}

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


static const aiScene* loadMesh(char* filename,  aiVector3D &scene_min, aiVector3D &scene_max, aiVector3D &scene_center) {

	const aiScene* scene = aiImportFile(filename, aiProcessPreset_TargetRealtime_MaxQuality);

	if (!scene) {
		cout << "## ERROR loading mesh" << endl;
		exit(-1);
	}

	aiMatrix4x4 trafo;
	aiIdentityMatrix4(&trafo);

	aiVector3D min, max;

	scene_min.x = scene_min.y = scene_min.z = FLT_MAX;
	scene_max.x = scene_max.y = scene_max.z = -FLT_MAX;

	get_bounding_box_for_node(scene, scene->mRootNode, &scene_min, &scene_max, &trafo);

	scene_center.x = (scene_min.x + scene_max.x) / 2.0f;
	scene_center.y = (scene_min.y + scene_max.y) / 2.0f;
	scene_center.z = (scene_min.z + scene_max.z) / 2.0f;

	scene_min.x *= 1.2;
	scene_min.y *= 1.2;
	scene_min.z *= 1.2;
	scene_max.x *= 1.2;
	scene_max.y *= 1.2;
	scene_max.z *= 1.2;

	cout << "Bounding Box: " << " ( " << scene_min.x << " , " << scene_min.y << " , " << scene_min.z << " ) - ( "
		<< scene_max.x << " , " << scene_max.y << " , " << scene_max.z << " )" << endl;
	cout << "Bounding Box: " << " ( " << scene_center.x << " , " << scene_center.y << " , " << scene_center.z << " )" << endl;

	return scene;
}

int main(int argc, char *argv[]) {

	GLFWwindow* window;

	char *meshFilename = "bunny.obj";

	window = initGLFW(argv[0], winWidth, winHeight);

	initASSIMP();
	initGL(window);
	initShaders();

	scene = loadMesh(meshFilename, scene_min, scene_max, scene_center);
	createVBOs(scene, vboVertices, vboColors, vboNormals);

	createAxis();
		
	criarBuffers();
	GLFW_MainLoop(window);

	glfwDestroyWindow(window);
	glfwTerminate();

	aiReleaseImport(scene);

	aiDetachAllLogStreams();

	exit(EXIT_SUCCESS);
}
