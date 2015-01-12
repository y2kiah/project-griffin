//#include <cinder/app/Renderer.h>

#include <vector>
#include <fstream>
#include <algorithm>
#include <GL/glew.h>
//#include <gl/glcorearb.h>
#include <glm/glm.hpp>
#include "Render.h"
#include <SDL_log.h>
#include <resource/ResourceLoader.h>
#include <render/texture/Texture2D_GL.h>

GLuint vertexArrayID;
GLuint vertexbuffer;

struct vertex_pcuv {
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 uv;
};

// An array of 3 vectors which represents 3 vertices
/*static const GLfloat g_vertex_buffer_data[] = {
	-1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
};*/
static const vertex_pcuv g_vertex_buffer_data[] = {
	{ { -1.0f, -1.0f, 0.0f }, { 1, 0, 0 }, { 0, 0 } },
	{ { -1.0f,  1.0f, 0.0f }, { 0, 1, 0 }, { 0, 1 } },
	{ {  1.0f,  1.0f, 0.0f }, { 0, 0, 1 }, { 1, 1 } },
	{ {  1.0f, -1.0f, 0.0f }, { 1, 1, 1 }, { 1, 0 } }
};

namespace griffin {
	namespace render {

		using std::vector;
		using std::move;

		// Forward Declarations
		bool loadTexturesTemp();

		// Global Variables
		weak_ptr<resource::ResourceLoader> g_loaderPtr;
		resource::ResourceHandle<Texture2D_GL> g_textureHandleTemp;

		// Functions

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

			loadTexturesTemp();
		}


		void renderFrame(double interpolation) {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// 1st attribute buffer : vertices
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
			glVertexAttribPointer(
				0,                  // attribute 0. position
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				sizeof(vertex_pcuv),// stride
				(void*)0 // array buffer offset
				);
			glVertexAttribPointer(
				1,                  // attribute 1. color
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				sizeof(vertex_pcuv),// stride
				(void*)offsetof(vertex_pcuv, color) // array buffer offset
				);
			glVertexAttribPointer(
				2,                  // attribute 2. uv
				2,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				sizeof(vertex_pcuv),// stride
				(void*)offsetof(vertex_pcuv, uv) // array buffer offset
				);

			// bind the texture
			auto loader = g_loaderPtr.lock();
			if (!loader) { return; }
			auto fTex = loader->getResource<Texture2D_GL>(g_textureHandleTemp);
			try {
				fTex.get()->getResource<Texture2D_GL>().bindToSampler(GL_TEXTURE0);
			}
			catch (std::exception& ex) {
				SDL_Log(ex.what());
			}

			// Draw the triangle !
			glDrawArrays(GL_POINTS, 0, 4); // Starting from vertex 0; 4 vertices total -> 2 triangles
			
			glEnable(GL_PROGRAM_POINT_SIZE);
			glPointSize(10);

			glDisableVertexAttribArray(0);
		}

		bool loadTexturesTemp()
		{
			using namespace resource;

			auto loader = g_loaderPtr.lock();

			if (loader) {
				auto textureResourceBuilder = [](DataPtr&& data, size_t size) {
					Texture2D_GL tex(move(data), size);
					return move(tex);
				};

				auto textureResourceCallback = [](const ResourcePtr& resourcePtr, Id_T handle, size_t size) {
					Texture2D_GL& tex = resourcePtr->getResource<Texture2D_GL>();
					// the unique_ptr of data is stored within the texture, this call deletes the data after
					// sending texture to OpenGL
					tex.loadFromInternalMemory();
				};

/*				g_textureHandleTemp = loader->load<Texture2D_GL>(L"../vendor/soil/img_test.png", textureResourceBuilder, textureResourceCallback);
				auto& texHandle = g_textureHandleTemp;
				try {
					texHandle.resourceId.wait(); // this blocks until the resource is built

					SDL_Log("Id = %llu", texHandle.value());
					SDL_Log("index = %u", texHandle.resourceId.get().index);
					SDL_Log("typeid = %u", texHandle.resourceId.get().typeId);
					SDL_Log("gen = %u", texHandle.resourceId.get().generation);
					SDL_Log("free = %u", texHandle.resourceId.get().free);
				}
				catch (std::runtime_error& ex) {
					SDL_Log(ex.what());
					return false;
				}

				loader->executeCallbacks();          // this runs the callback on this thread to send the texture to GL

				return true;
*/
			}

			return false;
		}


		unsigned int loadShaders(string vertexFilePath, string fragmentFilePath)
		{
			// Create the shaders
			GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
			GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

			// Read the Vertex Shader code from the file
			string vertexShaderCode;
			std::ifstream vertexShaderStream(vertexFilePath, std::ios::in);
			if (!vertexShaderStream.is_open()) {
				throw(std::runtime_error("Error opening file " + vertexFilePath));
			}
			string line = "";
			while (getline(vertexShaderStream, line)) {
				vertexShaderCode += "\n" + line;
			}
			vertexShaderStream.close();

			// Read the Fragment Shader code from the file
			string fragmentShaderCode;
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
			if (infoLogLength > 0) {
				vector<char> vertexShaderErrorMessage(infoLogLength);
				glGetShaderInfoLog(vertexShaderID, infoLogLength, NULL, &vertexShaderErrorMessage[0]);
				SDL_Log("%s\n", &vertexShaderErrorMessage[0]);
				fprintf(stdout, "%s\n", &vertexShaderErrorMessage[0]);
			}

			// Compile Fragment Shader
			SDL_Log("Compiling shader : %s\n", fragmentFilePath.c_str());
			fprintf(stdout, "Compiling shader : %s\n", fragmentFilePath.c_str());
			char const * fragmentSourcePointer = fragmentShaderCode.c_str();
			glShaderSource(fragmentShaderID, 1, &fragmentSourcePointer, NULL);
			glCompileShader(fragmentShaderID);

			// Check Fragment Shader
			glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &result);
			glGetShaderiv(fragmentShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
			if (infoLogLength > 0) {
				vector<char> fragmentShaderErrorMessage(infoLogLength);
				glGetShaderInfoLog(fragmentShaderID, infoLogLength, NULL, &fragmentShaderErrorMessage[0]);
				SDL_Log("%s\n", &fragmentShaderErrorMessage[0]);
				fprintf(stdout, "%s\n", &fragmentShaderErrorMessage[0]);
			}

			// Link the program
			fprintf(stdout, "Linking program\n");
			GLuint programID = glCreateProgram();
			glAttachShader(programID, vertexShaderID);
			glAttachShader(programID, fragmentShaderID);
			glLinkProgram(programID);

			// Check the program
			glGetProgramiv(programID, GL_LINK_STATUS, &result);
			glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
			vector<char> programErrorMessage(std::max(infoLogLength, 1));
			glGetProgramInfoLog(programID, infoLogLength, NULL, &programErrorMessage[0]);
			SDL_Log("%s\n", &programErrorMessage[0]);
			fprintf(stdout, "%s\n", &programErrorMessage[0]);

			glDeleteShader(vertexShaderID);
			glDeleteShader(fragmentShaderID);

			return programID;
		}



	}
}