#include <iostream>
#include <cstdio>
#include <cstdlib>

#include <GL/glew.h>



#include "initShaders.h"


/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

// Create a NULL-terminated string by reading the provided file
static char* readShaderSource(const char* shaderFile) {

	FILE* fp = fopen(shaderFile, "r");

	if (fp == NULL) { return NULL; }

	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);

	fseek(fp, 0L, SEEK_SET);
	char* buf = new char[size + 1];
	fread(buf, 1, size, fp);

	buf[size] = '\0';
	fclose(fp);

	return buf;
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

// Create a GLSL program object from vertex and fragment shader files

GLuint InitComputeShader(const char* compShader) {
	tShader shader = { compShader, GL_COMPUTE_SHADER, NULL };

	GLuint program = glCreateProgram();

	Shader& s = shader;
	s.source = readShaderSource(s.filename);
	if (shader.source == NULL) {
		std::cerr << "Failed to read " << s.filename << std::endl;
		exit(EXIT_FAILURE);
	}

	GLuint shaderBin = glCreateShader(s.type);
	glShaderSource(shaderBin, 1, (const GLchar**)&s.source, NULL);
	glCompileShader(shaderBin);

	GLint  compiled;
	glGetShaderiv(shaderBin, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		std::cerr << s.filename << " failed to compile:" << std::endl;
		GLint  logSize;
		glGetShaderiv(shaderBin, GL_INFO_LOG_LENGTH, &logSize);
		char* logMsg = new char[logSize];
		glGetShaderInfoLog(shaderBin, logSize, NULL, logMsg);
		std::cerr << logMsg << std::endl;
		delete[] logMsg;
		exit(EXIT_FAILURE);
	}

	delete[] s.source;
	//s.source = '\0';

	glAttachShader(program, shaderBin);

	/* link  and error check */
	glLinkProgram(program);

	GLint  linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked) {
		std::cerr << "Shader program failed to link" << std::endl;
		GLint  logSize;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize);
		char* logMsg = new char[logSize];
		glGetProgramInfoLog(program, logSize, NULL, logMsg);
		std::cerr << logMsg << std::endl;
		delete[] logMsg;

		exit(EXIT_FAILURE);
	}

	return program;
}

GLuint InitShader(const char* vShaderFile, const char* fShaderFile) {

	tShader shaders[2] = { { vShaderFile, GL_VERTEX_SHADER, NULL },
	{ fShaderFile, GL_FRAGMENT_SHADER, NULL }
	};

	GLuint program = glCreateProgram();

	for (int i = 0; i < 2; ++i) {
		Shader& s = shaders[i];
		s.source = readShaderSource(s.filename);
		if (shaders[i].source == NULL) {
			std::cerr << "Failed to read " << s.filename << std::endl;
			exit(EXIT_FAILURE);
		}

		GLuint shader = glCreateShader(s.type);
		glShaderSource(shader, 1, (const GLchar**)&s.source, NULL);
		glCompileShader(shader);

		GLint  compiled;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (!compiled) {
			std::cerr << s.filename << " failed to compile:" << std::endl;
			GLint  logSize;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
			char* logMsg = new char[logSize];
			glGetShaderInfoLog(shader, logSize, NULL, logMsg);
			std::cerr << logMsg << std::endl;
			delete[] logMsg;

			exit(EXIT_FAILURE);
		}

		delete[] s.source;
		//s.source = '\0';

		glAttachShader(program, shader);
	}

	/* link  and error check */
	glLinkProgram(program);

	GLint  linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked) {
		std::cerr << "Shader program failed to link" << std::endl;
		GLint  logSize;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize);
		char* logMsg = new char[logSize];
		glGetProgramInfoLog(program, logSize, NULL, logMsg);
		std::cerr << logMsg << std::endl;
		delete[] logMsg;

		exit(EXIT_FAILURE);
	}

	return program;
}
