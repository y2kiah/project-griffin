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
#include <render/material/ShaderProgram_GL.h>


namespace griffin {
	namespace render {

		using std::wstring;
		using std::vector;
		using std::move;

		struct vertex_pcuv {
			glm::vec3 position;
			glm::vec3 color;
			glm::vec2 uv;
		};

		// Forward Declarations

		bool loadTexturesTemp();
		bool loadShadersTemp(wstring, wstring);

		// Global Variables

		static const vertex_pcuv g_vertex_buffer_data[] = {
			{ { -1.0f, 1.0f, 0.0f }, { 1, 0, 0 }, { 0, 0 } },
			{ { -1.0f, -1.0f, 0.0f }, { 0, 1, 0 }, { 0, 1 } },
			{ { 1.0f, 1.0f, 0.0f }, { 0, 0, 1 }, { 1, 0 } },
			{ { 1.0f, -1.0f, 0.0f }, { 1, 1, 1 }, { 1, 1 } }
		};
		
		weak_ptr<resource::ResourceLoader> g_loaderPtr;

		// TEMP
		resource::ResourceHandle<Texture2D_GL> g_textureHandleTemp;
		std::shared_ptr<ShaderProgram_GL> g_tempShaderProgramPtr;
		GLuint vertexArrayId = 0;
		GLuint vertexbuffer = 0;
		GLuint programId = 0;

		// Functions

		void initRenderData() {
			glGenVertexArrays(1, &vertexArrayId);
			glBindVertexArray(vertexArrayId);

			// Generate 1 buffer, put the resulting identifier in vertexbuffer
			glGenBuffers(1, &vertexbuffer);

			// The following commands will talk about our 'vertexbuffer' buffer
			glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);

			// Give our vertices to OpenGL.
			glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

			loadShadersTemp(L"shaders/SimpleVertexShader.glsl",
							L"shaders/SimpleFragmentShader.glsl");

			loadTexturesTemp();
		}


		void renderFrame(double interpolation) {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			g_tempShaderProgramPtr->useProgram();

			glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);

			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);
			
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
				(void*)12 // array buffer offset
				);
			glVertexAttribPointer(
				2,                  // attribute 2. uv
				2,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				sizeof(vertex_pcuv),// stride
				(void*)24 // array buffer offset
				);

			// bind the texture
			auto loader = g_loaderPtr.lock();
			if (!loader) { return; }
			try {
				auto fTex = loader->getResource<Texture2D_GL>(g_textureHandleTemp);
				fTex.get()->getResource<Texture2D_GL>().bindToSampler(GL_TEXTURE0);
				
				GLint diffuse = glGetUniformLocation(programId, "diffuse"); // <-- uniform locations could be stored in shaderprogram structure
				glUniform1i(diffuse, 0);
			}
			catch (...) {}

			// Draw the triangle !
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // Starting from vertex 0; 4 vertices total -> 2 triangles
			
			glEnable(GL_PROGRAM_POINT_SIZE);
			glPointSize(10);

			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(2);
		}

		bool loadTexturesTemp()
		{
			using namespace resource;

			auto loader = g_loaderPtr.lock();

			if (loader) {
				auto textureResourceBuilder = [](DataPtr data, size_t size) {
					Texture2D_GL tex(move(data), size);
					SDL_Log("building texture of size %d", size);
					return tex;
				};

				auto textureResourceCallback = [](const ResourcePtr& resourcePtr, Id_T handle, size_t size) {
					Texture2D_GL& tex = resourcePtr->getResource<Texture2D_GL>();
					SDL_Log("callback texture of size %d", size);
					// the unique_ptr of data is stored within the texture, this call deletes the data after
					// sending texture to OpenGL
					tex.loadFromInternalMemory();
				};

				g_textureHandleTemp = loader->load<Texture2D_GL>(L"../vendor/soil/img_test.png", Cache_Materials_T,
																 textureResourceBuilder, textureResourceCallback);
				auto& texHandle = g_textureHandleTemp;
				try {
					texHandle.resourceId.wait(); // this blocks until the resource is built

					loader->executeCallbacks();  // this runs the callback on this thread to send the texture to GL

					SDL_Log("Id = %llu", texHandle.value());
					SDL_Log("index = %u", texHandle.resourceId.get().index);
					SDL_Log("typeid = %u", texHandle.resourceId.get().typeId);
					SDL_Log("gen = %u", texHandle.resourceId.get().generation);
					SDL_Log("free = %u", texHandle.resourceId.get().free);

					return true;
				}
				catch (std::runtime_error& ex) {
					SDL_Log(ex.what());
				}
			}

			return false;
		}


		bool loadShadersTemp(wstring vertexFilePath, wstring fragmentFilePath)
		{
			using namespace resource;

			auto loader = g_loaderPtr.lock();

			if (loader) {
				auto shaderResourceBuilder = [](DataPtr data, size_t size) {
					string shaderCode(reinterpret_cast<char*>(data.get()), size);
					return Shader_GL(shaderCode);
				};

				auto vertexHandle = loader->load<Shader_GL>(vertexFilePath, Cache_Materials_T, shaderResourceBuilder,
					[](const ResourcePtr& resourcePtr, Id_T handle, size_t size) {
						Shader_GL& shader = resourcePtr->getResource<Shader_GL>();
						auto ok = shader.compileShader(GL_VERTEX_SHADER);
						if (!ok) {
							throw std::runtime_error("vertex shader compilation failed");
						}
					});
				auto fragmentHandle = loader->load<Shader_GL>(fragmentFilePath, Cache_Materials_T, shaderResourceBuilder,
					[](const ResourcePtr& resourcePtr, Id_T handle, size_t size) {
						Shader_GL& shader = resourcePtr->getResource<Shader_GL>();
						auto ok = shader.compileShader(GL_FRAGMENT_SHADER);
						if (!ok) {
							throw std::runtime_error("fragment shader compilation failed");
						}
					});
				
				try {
					auto vertexResource = loader->getResource(vertexHandle);
					auto fragResource = loader->getResource(fragmentHandle);
					loader->executeCallbacks(); // this runs the callback on this thread to compile the shaders

					// create the program
					g_tempShaderProgramPtr = std::make_shared<ShaderProgram_GL>(vertexResource.get()->getResource<Shader_GL>(),
																				fragResource.get()->getResource<Shader_GL>());
					bool ok = g_tempShaderProgramPtr->linkProgram();
					
					if (ok) {
						//auto programHandle = loader->addToCache<ShaderProgram_GL>(shaderProgramPtr, Cache_Materials_T);
						return true;
					}
				}
				catch (std::runtime_error& ex) {
					SDL_Log(ex.what());
				}
			}

			return false;
		}



	}
}