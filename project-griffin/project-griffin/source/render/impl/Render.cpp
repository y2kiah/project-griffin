#include <vector>
#include <fstream>
#include <algorithm>
#include <GL/glew.h>
//#include <gl/glcorearb.h>
#include <glm/glm.hpp>
#include "../Render.h"
#include <SDL_log.h>
#include <resource/ResourceLoader.h>
#include "../ShaderProgramLayouts_GL.h"

#include <render/model/Mesh_GL.h>
#include <render/model/ModelImport_Assimp.h>
#include <render/RenderTarget_GL.h>

namespace griffin {
	namespace render {

		using std::wstring;
		using std::string;
		using std::vector;
		using std::move;

		// Forward Declarations

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

		bool DeferredRenderer_GL::init(int viewportWidth, int viewportHeight)
		{
			// get the resource loader
			using namespace resource;
			auto loader = g_loaderPtr.lock();
			if (!loader) {
				throw std::runtime_error("no resource loader");
			}

			// initialize the g-buffer render target
			if (!m_gbuffer.init(viewportWidth, viewportHeight)) {
				throw std::runtime_error("Cannot initialize renderer");
			}

			// build fullscreen quad vertex buffer
			m_fullScreenQuad.loadFromMemory(reinterpret_cast<const unsigned char*>(g_fullScreenQuadBufferData),
											sizeof(g_fullScreenQuadBufferData));

			// build VAO for fullscreen quad
			glGenVertexArrays(1, &m_glQuadVAO);
			glBindVertexArray(m_glQuadVAO);
			m_fullScreenQuad.bind();
			glEnableVertexAttribArray(VertexLayout_Position);
			glVertexAttribPointer(VertexLayout_Position, 3, GL_FLOAT, GL_FALSE, 0, 0);

			// load shader programs for deferred rendering
			// hold a shared_ptr to these shader programs so they never fall out of cache

			auto fsq  = loadShaderProgram(L"shaders/fullscreenQuad.glsl");
			auto ssao = loadShaderProgram(L"shaders/ssao.glsl");
			auto mrt  = loadShaderProgram(L"shaders/ads.glsl"); // temporarily ads.glsl
			//L"shaders/linearDepth.glsl"
			//L"shaders/atmosphere/earth.glsl"
			//L"shaders/atmosphere/atmosphere.glsl"
			//L"shaders/SimpleShader.glsl"

			m_fullScreenQuadProgram = loader->getResource(fsq).get(); // wait on the futures and assign shared_ptrs
			m_ssaoProgram = loader->getResource(ssao).get();
			m_mrtProgram = loader->getResource(mrt).get();

			return true;
		}

		void DeferredRenderer_GL::drawFullscreenQuad(/*Viewport*/) const
		{
			glBindVertexArray(m_glQuadVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		DeferredRenderer_GL::~DeferredRenderer_GL()
		{
			if (m_glQuadVAO != 0) {
				glDeleteVertexArrays(1, &m_glQuadVAO);
			}
		}


		// Functions

		void RenderSystem::init(int viewportWidth, int viewportHeight) {
			//loadModelTemp("data/models/ship.dae");
			//loadModelTemp("data/models/riggedFighter.dae");
			loadModelTemp("data/models/landing platform.dae");
			//loadModelTemp("data/models/quadcopter2.dae");
			//loadModelTemp("data/models/cube.dae");
			//loadModelTemp("data/models/untitled.blend");

			camera = std::make_unique<CameraPersp>(viewportWidth, viewportHeight, 60.0f, 0.1f, 10000.0f);
		}


		void DeferredRenderer_GL::renderFrame(double interpolation) {
			glClearColor(0.2f, 0.4f, 0.8f, 1.0f); // temp
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			auto loader = g_loaderPtr.lock();
			auto programRes = loader->getResource<ShaderProgram_GL>(g_programHandleTemp);
			auto& program = programRes.get()->getResource<ShaderProgram_GL>();
			program.useProgram();
			auto programId = program.getProgramId();

			//camera->setEyePoint({ 0.0f, 100.0f, 200.0f });
			//camera->setEyePoint({ 10.0f, 0.0f, 90.0f });
			//camera->setEyePoint({ 0.0f, 40.0f, -40.0f });
			camera->setEyePoint({ 120.0f, 40.0f, 0.0f });
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
			GLint modelMatLoc     = glGetUniformLocation(programId, "modelToWorld");
			GLint modelViewMatLoc = glGetUniformLocation(programId, "modelView");
			GLint viewProjMatLoc  = glGetUniformLocation(programId, "viewProjection");
			GLint mvpMatLoc       = glGetUniformLocation(programId, "modelViewProjection");
			GLint normalMatLoc    = glGetUniformLocation(programId, "normalMatrix");
			
			GLint ambientLoc      = glGetUniformLocation(programId, "materialKa");
			GLint diffuseLoc      = glGetUniformLocation(programId, "materialKd");
			GLint specularLoc     = glGetUniformLocation(programId, "materialKs");
			GLint shininessLoc    = glGetUniformLocation(programId, "materialShininess");
			
			glUniformMatrix4fv(viewProjMatLoc, 1, GL_FALSE, &viewProjMat[0][0]);

			// bind the texture
			/*if (!loader) { return; }
			try {
				auto fTex = loader->getResource<Texture2D_GL>(g_textureHandleTemp);
				fTex.get()->getResource<Texture2D_GL>().bind(GL_TEXTURE0);
				
				GLint diffuse = glGetUniformLocation(programId, "diffuse"); // <-- uniform locations could be stored in shaderprogram structure
				glUniform1i(diffuse, 0);
			}
			catch (...) {}*/

			// draw the test mesh
			g_tempMesh->draw(modelMatLoc, modelViewMatLoc, mvpMatLoc, normalMatLoc,
							 ambientLoc, diffuseLoc, specularLoc, shininessLoc,
							 viewMat, viewProjMat); // temporarily passing in the modelMatLoc
		}


		ResourceHandle<Texture2D_GL> loadTexture(wstring texturePath, CacheType cache)
		{
			using namespace resource;

			auto loader = g_loaderPtr.lock();

			if (!loader) {
				throw std::runtime_error("no resource loader");
			}

			auto textureResourceBuilder = [](DataPtr data, size_t size) {
				Texture2D_GL tex(move(data), size);
				SDL_Log("building texture of size %d", size);
				return tex;
			};

			// need a way to specify thread affinity for the callback so it knows to run on update or render thread
			auto textureResourceCallback = [](const ResourcePtr& resourcePtr, Id_T handle, size_t size) {
				Texture2D_GL& tex = resourcePtr->getResource<Texture2D_GL>();
				SDL_Log("callback texture of size %d", size);
				// the unique_ptr of data is stored within the texture, this call deletes the data after
				// sending texture to OpenGL
				tex.loadFromInternalMemory();
			};

			return loader->load<Texture2D_GL>(texturePath, cache, textureResourceBuilder, textureResourceCallback);
		}


		ResourceHandle<ShaderProgram_GL> loadShaderProgram(wstring programPath, resource::CacheType cache)
		{
			using namespace resource;

			auto loader = g_loaderPtr.lock();

			if (!loader) {
				throw std::runtime_error("no resource loader");
			}

			auto shaderResourceBuilder = [](DataPtr data, size_t size) {
				string shaderCode(reinterpret_cast<char*>(data.get()), size);
				return ShaderProgram_GL(shaderCode);
			};

			auto shaderResourceCallback = [](const ResourcePtr& resourcePtr, Id_T handle, size_t size) {
				ShaderProgram_GL& program = resourcePtr->getResource<ShaderProgram_GL>();
				auto ok = program.compileAndLinkProgram();
				if (!ok) {
					throw std::runtime_error("program compilation/linking failed");
				}
			};

			return loader->load<ShaderProgram_GL>(programPath, cache, shaderResourceBuilder, shaderResourceCallback);
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