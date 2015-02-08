#include <vector>
#include <fstream>
#include <algorithm>
#include <GL/glew.h>
//#include <gl/glcorearb.h>
#include <glm/glm.hpp>
#include "../Render.h"
#include <SDL_log.h>
#include <resource/ResourceLoader.h>
#include <render/texture/Texture2D_GL.h>
#include "../ShaderProgram_GL.h"
#include "../ShaderProgramLayouts_GL.h"

#include <render/model/Mesh_GL.h>
#include <render/model/ModelImport_Assimp.h>
#include <render/Camera.h>

namespace griffin {
	namespace render {

		using std::wstring;
		using std::string;
		using std::vector;
		using std::move;

		// Forward Declarations

		bool loadTexturesTemp();
		bool loadShadersTemp(wstring, wstring);
		bool loadModelTemp(string);

		// Global Variables

		weak_ptr<resource::ResourceLoader> g_loaderPtr;

		// TEMP
		resource::ResourceHandle<Texture2D_GL> g_textureHandleTemp;
		std::shared_ptr<ShaderProgram_GL> g_tempShaderProgramPtr = nullptr;
		std::unique_ptr<Mesh_GL> g_tempMesh = nullptr;
		GLuint programId = 0;
		std::unique_ptr<CameraPersp> camera;

		// Functions

		void initRenderData(int viewportWidth, int viewportHeight) {
			loadShadersTemp(L"shaders/SimpleVertexShader.glsl",
							L"shaders/SimpleFragmentShader.glsl");

			loadTexturesTemp();
			loadModelTemp("data/models/landing platform.dae");

			camera = std::make_unique<CameraPersp>(viewportWidth, viewportHeight, 60.0f, 0.1f, 10000.0f);
		}


		void renderFrame(double interpolation) {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			g_tempShaderProgramPtr->useProgram();

			// bind uniforms
			camera->setEyePoint(vec3(0.0f, 0.0f, -100.0f));
			camera->lookAt(vec3(0.0f, 0.0f, 0.0f));
			mat4 mvp(camera->getProjectionMatrix() * camera->getModelViewMatrix());
			glUniformMatrix4fv(UniformLayout_ModelView, 1, GL_FALSE, &camera->getModelViewMatrix()[0][0]);
			glUniformMatrix4fv(UniformLayout_Projection, 1, GL_FALSE, &camera->getProjectionMatrix()[0][0]);
			glUniformMatrix4fv(UniformLayout_ModelViewProjection, 1, GL_FALSE, &mvp[0][0]);

			// bind the texture
			auto loader = g_loaderPtr.lock();
			if (!loader) { return; }
			try {
				auto fTex = loader->getResource<Texture2D_GL>(g_textureHandleTemp);
				fTex.get()->getResource<Texture2D_GL>().bind(GL_TEXTURE0);
				
				GLint diffuse = glGetUniformLocation(programId, "diffuse"); // <-- uniform locations could be stored in shaderprogram structure
				glUniform1i(diffuse, 0);
			}
			catch (...) {}

			// draw the test mesh
			g_tempMesh->draw();
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


		bool loadModelTemp(string modelFilePath)
		{
			g_tempMesh = importModelFile(modelFilePath);

			return (g_tempMesh != false);

			/*using namespace resource;

			auto loader = g_loaderPtr.lock();

			if (loader) {
				auto modelResourceBuilder = [](DataPtr data, size_t size) {
					return Mesh_GL(std::move(data), size);
				};

				auto vertexHandle = loader->load<Shader_GL>(vertexFilePath, Cache_Materials_T, shaderResourceBuilder,
															[](const ResourcePtr& resourcePtr, Id_T handle, size_t size) {
					Shader_GL& shader = resourcePtr->getResource<Shader_GL>();
					auto ok = shader.compileShader(GL_VERTEX_SHADER);
					if (!ok) {
						throw std::runtime_error("vertex shader compilation failed");
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

			return false;*/
		}
	}
}