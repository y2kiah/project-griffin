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
#include <render/RenderTarget_GL.h>

namespace griffin {
	namespace render {

		using std::wstring;
		using std::string;
		using std::vector;
		using std::move;

		// Forward Declarations

		bool loadTexturesTemp();
		bool loadShadersTemp(wstring);
		bool loadModelTemp(string);

		// Global Variables

		weak_ptr<resource::ResourceLoader> g_loaderPtr;

		const float g_fullScreenQuadBufferData[] = {
			-1.0f, -1.0f, 0.0f,
			 1.0f, -1.0f, 0.0f,
			-1.0f,  1.0f, 0.0f,
			-1.0f,  1.0f, 0.0f,
			 1.0f, -1.0f, 0.0f,
			 1.0f,  1.0f, 0.0f,
		};

		// TEMP
		resource::ResourceHandle<Texture2D_GL> g_textureHandleTemp;
		resource::ResourceHandle<ShaderProgram_GL> g_programHandleTemp;
		std::unique_ptr<Mesh_GL> g_tempMesh = nullptr;
		std::unique_ptr<CameraPersp> camera = nullptr;


		// class RenderQueue

		void RenderQueue::addRenderEntry(RenderQueueKey sortKey, RenderEntry&& entry)
		{
			m_entries.push_back(std::forward<RenderEntry>(entry));
			m_keys.push_back({ sortKey, static_cast<int>(m_entries.size()) });
		}

		void RenderQueue::sortRenderQueue()
		{
			std::sort(m_keys.begin(), m_keys.end(), [](const KeyType& a, const KeyType& b) {
				return (a.key.value < b.key.value);
			});
		}

		RenderQueue::~RenderQueue()
		{
			if (m_keys.capacity() > RESERVE_RENDER_QUEUE) {
				SDL_Log("check RESERVE_RENDER_QUEUE: original=%d, highest=%d", RESERVE_RENDER_QUEUE, m_keys.capacity());
			}
		}


		// class DeferredRenderer_GL

		bool DeferredRenderer_GL::init(int viewportWidth, int viewportHeight) {
			if (!m_gbuffer.init(viewportWidth, viewportHeight)) {
				throw std::runtime_error("Cannot initialize renderer");
			}

			m_fullScreenQuad.loadFromMemory(reinterpret_cast<const unsigned char*>(g_fullScreenQuadBufferData),
											sizeof(g_fullScreenQuadBufferData));
		}

		// Functions

		void initRenderData(int viewportWidth, int viewportHeight) {
			//loadShadersTemp(L"shaders/ssao.glsl");
			//loadShadersTemp(L"shaders/linearDepth.glsl");
			loadShadersTemp(L"shaders/atmosphere/earth.glsl");
			//loadShadersTemp(L"shaders/atmosphere/atmosphere.glsl");
			//loadShadersTemp(L"shaders/ads.glsl");
			loadShadersTemp(L"shaders/SimpleShader.glsl");

			loadTexturesTemp();
			//loadModelTemp("data/models/ship.dae");
			//loadModelTemp("data/models/landing platform.dae");
			loadModelTemp("data/models/quadcopter2.dae");
			//loadModelTemp("data/models/cube.dae");

			camera = std::make_unique<CameraPersp>(viewportWidth, viewportHeight, 60.0f, 0.1f, 10000.0f);
		}


		void renderFrame(double interpolation) {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			auto loader = g_loaderPtr.lock();
			auto programRes = loader->getResource<ShaderProgram_GL>(g_programHandleTemp);
			auto& program = programRes.get()->getResource<ShaderProgram_GL>();
			program.useProgram();
			auto programId = program.getProgramId();

			camera->setEyePoint({ 0.0f, 40.0f, -40.0f });//{ 120.0f, 40.0f, 0.0f });
			camera->lookAt({ 0.0f, 0.0f, 0.0f });
			camera->setWorldUp({ 0.0f, 1.0f, 0.0f });
			camera->calcMatrices();
			mat4 viewMat(camera->getModelViewMatrix());
			mat4 viewProjMat(camera->getProjectionMatrix() * viewMat);

			/*camera->setTranslationYawPitchRoll({ 10.0f, 10.0f, 10.0f }, glm::radians(315.0f), glm::radians(45.0f), 0);
			//camera->lookAt({ 10.0f, 10.0f, 10.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
			camera->calcMatrices();
			mat4 model(1.0f);
			mat4 mvp(camera->getViewProjection() * model);
			*/

			//glUniformMatrix4fv(UniformLayout_ModelView, 1, GL_FALSE, &camera->getModelViewMatrix()[0][0]);
			//glUniformMatrix4fv(UniformLayout_Projection, 1, GL_FALSE, &camera->getProjectionMatrix()[0][0]);
			//glUniformMatrix4fv(UniformLayout_ModelViewProjection, 1, GL_FALSE, &mvp[0][0]);
			GLint modelMatLoc    = glGetUniformLocation(programId, "modelToWorld");
			GLint viewProjMatLoc = glGetUniformLocation(programId, "viewProjection");
			GLint normalMatLoc   = glGetUniformLocation(programId, "normalMatrix");
			GLint diffuseMatLoc  = glGetUniformLocation(programId, "diffuseColor");
			glUniformMatrix4fv(viewProjMatLoc, 1, GL_FALSE, &viewProjMat[0][0]);

			// bind the texture
			if (!loader) { return; }
			try {
				auto fTex = loader->getResource<Texture2D_GL>(g_textureHandleTemp);
				fTex.get()->getResource<Texture2D_GL>().bind(GL_TEXTURE0);
				
				GLint diffuse = glGetUniformLocation(programId, "diffuse"); // <-- uniform locations could be stored in shaderprogram structure
				glUniform1i(diffuse, 0);
			}
			catch (...) {}

			// draw the test mesh
			g_tempMesh->draw(modelMatLoc, normalMatLoc, diffuseMatLoc, viewMat); // temporarily passing in the modelMatLoc
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
					return true;
				}
				catch (std::runtime_error& ex) {
					SDL_Log(ex.what());
				}
			}

			return false;
		}


		bool loadShadersTemp(wstring programPath)
		{
			using namespace resource;

			auto loader = g_loaderPtr.lock();

			if (loader) {
				auto shaderResourceBuilder = [](DataPtr data, size_t size) {
					string shaderCode(reinterpret_cast<char*>(data.get()), size);
					return ShaderProgram_GL(shaderCode);
				};

				g_programHandleTemp = loader->load<ShaderProgram_GL>(programPath, Cache_Materials_T, shaderResourceBuilder,
					[](const ResourcePtr& resourcePtr, Id_T handle, size_t size) {
						ShaderProgram_GL& program = resourcePtr->getResource<ShaderProgram_GL>();
						auto ok = program.compileAndLinkProgram();
						if (!ok) {
							throw std::runtime_error("program compilation/linking failed");
						}
					});

				try {
					g_programHandleTemp.resourceId.wait();
					//auto programHandle = loader->addToCache<ShaderProgram_GL>(shaderProgramPtr, Cache_Materials_T);
					return true;
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