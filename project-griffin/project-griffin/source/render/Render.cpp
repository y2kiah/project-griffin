#include <vector>
#include <fstream>
#include <algorithm>
#include <GL/glew.h>
//#include <gl/glcorearb.h>
#include <glm/glm.hpp>
#include "Render.h"
#include <SDL_log.h>

GLuint vertexArrayID;
GLuint vertexbuffer;

// An array of 3 vectors which represents 3 vertices
static const GLfloat g_vertex_buffer_data[] = {
	-1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
};

void initRenderData() {
	glGenVertexArrays(1, &vertexArrayID);
	glBindVertexArray(vertexArrayID);

	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	glGenBuffers(1, &vertexbuffer);

	// The following commands will talk about our 'vertexbuffer' buffer
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);

	// Give our vertices to OpenGL.
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	GLuint programID = loadShaders("./data/shaders/SimpleVertexShader.glsl",
								   "./data/shaders/SimpleFragmentShader.glsl");
	glUseProgram(programID);
}

void renderFrame(double interpolation) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// 1st attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	// Draw the triangle !
	glDrawArrays(GL_TRIANGLES, 0, 3); // Starting from vertex 0; 3 vertices total -> 1 triangle

	glDisableVertexAttribArray(0);
}

unsigned int loadShaders(std::string vertexFilePath, std::string fragmentFilePath)
{
	// Create the shaders
	GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string vertexShaderCode;
	std::ifstream vertexShaderStream(vertexFilePath, std::ios::in);
	if (!vertexShaderStream.is_open()) {
		throw(std::runtime_error("Error opening file " + vertexFilePath));
	}
	std::string line = "";
	while (getline(vertexShaderStream, line)) {
		vertexShaderCode += "\n" + line;
	}
	vertexShaderStream.close();

	// Read the Fragment Shader code from the file
	std::string fragmentShaderCode;
	std::ifstream fragmentShaderStream(fragmentFilePath, std::ios::in);
	if (!fragmentShaderStream.is_open()) {
		throw(std::runtime_error("Error opening file " + fragmentFilePath));
	}
	line = "";
	while (getline(fragmentShaderStream, line)) {
		fragmentShaderCode += "\n" + line;
	}
	fragmentShaderStream.close();

	GLint result = GL_FALSE;
	int infoLogLength;

	// Compile Vertex Shader
	SDL_Log("Compiling shader : %s\n", vertexFilePath.c_str());
	printf("Compiling shader : %s\n", vertexFilePath.c_str());
	char const * vertexSourcePointer = vertexShaderCode.c_str();
	glShaderSource(vertexShaderID, 1, &vertexSourcePointer, NULL);
	glCompileShader(vertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &result);
	glGetShaderiv(vertexShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
	std::vector<char> vertexShaderErrorMessage(infoLogLength);
	glGetShaderInfoLog(vertexShaderID, infoLogLength, NULL, &vertexShaderErrorMessage[0]);
	SDL_Log("%s\n", &vertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &vertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	SDL_Log("Compiling shader : %s\n", fragmentFilePath.c_str());
	fprintf(stdout, "Compiling shader : %s\n", fragmentFilePath.c_str());
	char const * fragmentSourcePointer = fragmentShaderCode.c_str();
	glShaderSource(fragmentShaderID, 1, &fragmentSourcePointer, NULL);
	glCompileShader(fragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &result);
	glGetShaderiv(fragmentShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
	std::vector<char> fragmentShaderErrorMessage(infoLogLength);
	glGetShaderInfoLog(fragmentShaderID, infoLogLength, NULL, &fragmentShaderErrorMessage[0]);
	SDL_Log("%s\n", &fragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &fragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint programID = glCreateProgram();
	glAttachShader(programID, vertexShaderID);
	glAttachShader(programID, fragmentShaderID);
	glLinkProgram(programID);

	// Check the program
	glGetProgramiv(programID, GL_LINK_STATUS, &result);
	glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
	std::vector<char> programErrorMessage(std::max(infoLogLength, 1));
	glGetProgramInfoLog(programID, infoLogLength, NULL, &programErrorMessage[0]);
	SDL_Log("%s\n", &programErrorMessage[0]);
	fprintf(stdout, "%s\n", &programErrorMessage[0]);

	glDeleteShader(vertexShaderID);
	glDeleteShader(fragmentShaderID);

	return programID;
}