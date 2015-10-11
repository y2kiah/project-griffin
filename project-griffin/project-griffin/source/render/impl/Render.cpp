/**
* @file Render.cpp
* @author Jeff Kiah
*/
#include <vector>
#include <fstream>
#include <algorithm>
#include <GL/glew.h>
//#include <gl/glcorearb.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL_log.h>

#include <resource/ResourceLoader.h>
#include <render/Render.h>
#include <render/RenderResources.h>
#include <render/RenderHelpers.h>
#include <render/ShaderProgramLayouts_GL.h>
#include <render/model/Mesh_GL.h>
#include <render/model/ModelImport_Assimp.h>
#include <render/RenderTarget_GL.h>


namespace griffin {
	namespace render {

		using std::wstring;
		using std::string;
		using std::vector;
		using std::move;

		// Global Variables

		weak_ptr<resource::ResourceLoader> g_resourceLoader;

		// TEMP
		ResourcePtr		g_tempModel = nullptr;

		// defined in RenderHelpers.cpp
		extern float			g_fullScreenQuadBufferData[3 * 6];
		extern uint32_t			g_glQuadVAO;
		extern VertexBuffer_GL	g_fullScreenQuad;
		extern float			g_cubeBufferData[3 * 36];
		extern uint32_t			g_glCubeVAO;
		extern VertexBuffer_GL	g_cubeBuffer;


		// class RenderQueue

		void RenderQueue::addRenderEntry(RenderQueueKey sortKey, RenderEntry entry)
		{
			m_entries.push_back(std::move(entry));
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

		void DeferredRenderer_GL::init(int viewportWidth, int viewportHeight)
		{
			// get the resource loader
			using namespace resource;
			auto loader = g_resourceLoader.lock();
			if (!loader) {
				throw std::runtime_error("no resource loader");
			}

			// initialize the g-buffer render target
			if (!m_gbuffer.init(viewportWidth, viewportHeight)) {
				throw std::runtime_error("Cannot initialize renderer");
			}

			// initialize the fxaa colorBuffer
			if (!m_colorBuffer.init(viewportWidth, viewportHeight)) {
				throw std::runtime_error("Connot initialize color buffer");
			}
			m_colorBuffer.bind(RenderTarget_GL::Albedo_Displacement, GL_TEXTURE0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // fxaa sampler requires bilinear filtering
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			// load shader programs for deferred rendering
			// hold a shared_ptr to these shader programs so they never fall out of cache

			//auto fsq  = loadShaderProgram(L"shaders/fullscreenQuad.glsl");
			auto mrt  = loadShaderProgram(L"shaders/ads.glsl"); // temporarily ads.glsl
			auto sky  = loadShaderProgram(L"shaders/skybox.glsl");
			auto ssao = loadShaderProgram(L"shaders/ssao.glsl");
			auto atms = loadShaderProgram(L"shaders/atmosphere/atmosphere.glsl");
			auto fxaa = loadShaderProgram(L"shaders/fxaa.glsl");
			//L"shaders/linearDepth.glsl"
			//L"shaders/atmosphere/earth.glsl"
			//L"shaders/SimpleShader.glsl"

			auto nrml = loadTexture2D(L"textures/normal-noise.dds", CacheType::Cache_Permanent);

			//m_fullScreenQuadProgram = loader->getResource(fsq).get(); // wait on the futures and assign shared_ptrs
			m_mrtProgram = loader->getResource(mrt).get();
			m_skyboxProgram = loader->getResource(sky).get();
			m_ssaoProgram = loader->getResource(ssao).get();
			m_atmosphereProgram = loader->getResource(atms).get();
			m_fxaaProgram = loader->getResource(fxaa).get();

			// set up normal noise texture for ssao
			m_normalsTexture = loader->getResource(nrml).get();
			glBindTexture(GL_TEXTURE_2D, m_normalsTexture->getResource<Texture2D_GL>().getGLTexture());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			// Clear color of the render targets
			#ifdef _DEBUG
			glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
			#else
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			#endif

			// Enable stencil testing
			glEnable(GL_STENCIL_TEST);
			glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
			glStencilFunc(GL_ALWAYS, 1, 0xFF);
			glStencilMask(0x00); // writing turned off to start
		}


		// class DeferredRenderer_GL

		void DeferredRenderer_GL::renderViewport(ViewportParameters& viewportParams)
		{
			static float animTime = 0.0f; // TEMP

			// Start g-buffer rendering
			m_gbuffer.start();
			{
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
				
				glEnable(GL_DEPTH_TEST);

				glStencilFunc(GL_ALWAYS, 1, 0xFF);
				glStencilMask(0xFF);

				auto& program = m_mrtProgram.get()->getResource<ShaderProgram_GL>();
				program.useProgram();
				auto programId = program.getProgramId();

				//glUniformMatrix4fv(UniformLayout_ModelView, 1, GL_FALSE, &camera->getModelViewMatrix()[0][0]);
				//glUniformMatrix4fv(UniformLayout_Projection, 1, GL_FALSE, &camera->getProjectionMatrix()[0][0]);
				//glUniformMatrix4fv(UniformLayout_ModelViewProjection, 1, GL_FALSE, &mvp[0][0]);
				GLint modelMatLoc     = glGetUniformLocation(programId, "modelToWorld");
				GLint modelViewMatLoc = glGetUniformLocation(programId, "modelView");
				GLint viewProjMatLoc  = glGetUniformLocation(programId, "viewProjection");
				GLint mvpMatLoc       = glGetUniformLocation(programId, "modelViewProjection");
				GLint normalMatLoc    = glGetUniformLocation(programId, "normalMatrix");

				GLint frustumNearLoc  = glGetUniformLocation(programId, "frustumNear");
				GLint frustumFarLoc   = glGetUniformLocation(programId, "frustumFar");
				GLint inverseFrustumDistanceLoc = glGetUniformLocation(programId, "inverseFrustumDistance");

				GLint ambientLoc      = glGetUniformLocation(programId, "materialKa");
				GLint diffuseLoc      = glGetUniformLocation(programId, "materialKd");
				GLint specularLoc     = glGetUniformLocation(programId, "materialKs");
				GLint shininessLoc    = glGetUniformLocation(programId, "materialShininess");

				GLint diffuseMapLoc   = glGetUniformLocation(programId, "diffuseMap");

				glUniformMatrix4fv(viewProjMatLoc, 1, GL_FALSE, &viewportParams.viewProjMat[0][0]);

				glUniform1f(frustumNearLoc, viewportParams.nearClipPlane);
				glUniform1f(frustumFarLoc, viewportParams.farClipPlane);
				glUniform1f(inverseFrustumDistanceLoc, viewportParams.inverseFrustumDistance);
				
				// TEMP
				animTime += 0.001667f;
				if (animTime > 2.5f) {
					animTime = 0.0f;
				}

				// draw the test mesh
				if (g_tempModel) {
					auto& mdl = g_tempModel->getResource<Model_GL>();
					mdl.m_mesh.draw(modelMatLoc, modelViewMatLoc, mvpMatLoc, normalMatLoc,
									ambientLoc, diffuseLoc, specularLoc, shininessLoc,
									diffuseMapLoc, animTime,
									viewportParams.viewMat, viewportParams.projMat); // temporarily passing in the modelMatLoc
				}

				glDisable(GL_DEPTH_TEST);
				
				// Render Skybox
				if (m_skyboxTexture) {
					auto& program = m_skyboxProgram.get()->getResource<ShaderProgram_GL>();
					program.useProgram();
					auto programId = program.getProgramId();

					auto& skybox = m_skyboxTexture.get()->getResource<TextureCubeMap_GL>();
					skybox.bind(GL_TEXTURE0);

					GLint mvpMatLoc = glGetUniformLocation(programId, "modelViewProjection");
					GLint cubemapLoc = glGetUniformLocation(programId, "cubemap");

					glm::mat4 skyboxViewMat = viewportParams.viewMat;
					skyboxViewMat[3].xyz = 0.0f;
					glm::mat4 skyboxMVP = viewportParams.projMat * skyboxViewMat;

					glUniformMatrix4fv(mvpMatLoc, 1, GL_FALSE, &skyboxMVP[0][0]);
					glUniform1i(cubemapLoc, 0);

					glStencilFunc(GL_EQUAL, 0, 0xFF); // Pass test if stencil value is 0
					drawCube();
					glStencilFunc(GL_ALWAYS, 1, 0xFF);
				}
			}
			m_gbuffer.stop();
			// End g-buffer rendering

			// Start post-processing
			m_colorBuffer.start();
			{
				glClear(GL_COLOR_BUFFER_BIT);

				// SSAO
				auto& ssao = m_ssaoProgram.get()->getResource<ShaderProgram_GL>();
				ssao.useProgram();
				auto ssaoId = ssao.getProgramId();

				m_normalsTexture.get()->getResource<Texture2D_GL>().bind(GL_TEXTURE0);
				m_gbuffer.bind(RenderTarget_GL::Albedo_Displacement, GL_TEXTURE1);
				m_gbuffer.bind(RenderTarget_GL::Normal_Reflectance, GL_TEXTURE2);
				m_gbuffer.bind(RenderTarget_GL::Depth_Stencil, GL_TEXTURE3);
				m_gbuffer.bind(RenderTarget_GL::Position, GL_TEXTURE4);

				GLint normalNoiseLoc = glGetUniformLocation(ssaoId, "normalNoise"); // <-- uniform locations could be stored in shaderprogram structure
				GLint colorMapLoc    = glGetUniformLocation(ssaoId, "colorMap");
				GLint normalMapLoc   = glGetUniformLocation(ssaoId, "normalMap");
				GLint depthMapLoc    = glGetUniformLocation(ssaoId, "depthMap");
				GLint positionMapLoc = glGetUniformLocation(ssaoId, "positionMap");
				
				GLint cameraNearLoc  = glGetUniformLocation(ssaoId, "cameraNear");
				GLint cameraFarLoc   = glGetUniformLocation(ssaoId, "cameraFar");
				
				glUniform1i(normalNoiseLoc, 0);
				glUniform1i(colorMapLoc, 1);
				glUniform1i(normalMapLoc, 2);
				glUniform1i(depthMapLoc, 3);
				glUniform1i(positionMapLoc, 4);
				
				glUniform1f(cameraNearLoc, viewportParams.nearClipPlane);
				glUniform1f(cameraFarLoc, viewportParams.farClipPlane);

				drawFullscreenQuad();
			}
			m_colorBuffer.stop();

			{
				glClear(GL_COLOR_BUFFER_BIT);

				// FXAA
				auto& fxaa = m_fxaaProgram.get()->getResource<ShaderProgram_GL>();
				fxaa.useProgram();
				auto fxaaId = fxaa.getProgramId();

				// requires bilinear filter on the sampler
				m_colorBuffer.bind(RenderTarget_GL::Albedo_Displacement, GL_TEXTURE0);	// when SSAO on
				
				//m_gbuffer.bind(RenderTarget_GL::Albedo_Displacement, GL_TEXTURE0);		// when SSAO off, alpha channel must contain luma
				//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				GLint colorMapLoc = glGetUniformLocation(fxaaId, "colorMap");
				glUniform1i(colorMapLoc, 0);

				drawFullscreenQuad();
			}
			// End post-processing
		}

		DeferredRenderer_GL::~DeferredRenderer_GL()
		{
		}


		// class RenderSystem

		void RenderSystem::init(int viewportWidth, int viewportHeight) {
			// build fullscreen quad vertex buffer
			g_fullScreenQuad.loadFromMemory(reinterpret_cast<const unsigned char*>(g_fullScreenQuadBufferData),
											sizeof(g_fullScreenQuadBufferData));

			// build VAO for fullscreen quad
			glGenVertexArrays(1, &g_glQuadVAO);
			glBindVertexArray(g_glQuadVAO);
			g_fullScreenQuad.bind();
			glEnableVertexAttribArray(VertexLayout_Position);
			glVertexAttribPointer(VertexLayout_Position, 3, GL_FLOAT, GL_FALSE, 0, 0);

			// build cube vertex buffer
			g_cubeBuffer.loadFromMemory(reinterpret_cast<const unsigned char*>(g_cubeBufferData),
										sizeof(g_cubeBufferData));

			// build VAO for cube
			glGenVertexArrays(1, &g_glCubeVAO);
			glBindVertexArray(g_glCubeVAO);
			g_cubeBuffer.bind();
			glEnableVertexAttribArray(VertexLayout_Position);
			glVertexAttribPointer(VertexLayout_Position, 3, GL_FLOAT, GL_FALSE, 0, 0);

			// init the renderers
			m_deferredRenderer.init(viewportWidth, viewportHeight);

			// set up default viewport matrices
			ViewportParameters defaultView{};
			defaultView.nearClipPlane = 0.1f; //-1.0f;
			defaultView.farClipPlane  = 100000.0f; //1.0f;
			defaultView.frustumDistance = defaultView.farClipPlane - defaultView.nearClipPlane;
			defaultView.inverseFrustumDistance = 1.0f / defaultView.frustumDistance;
			defaultView.viewMat = glm::lookAt(glm::dvec3{ 120.0, 40.0, 0 }, glm::dvec3{ 0, 0, 0 }, glm::dvec3{ 0, 0, 1.0 });
								  //glm::lookAt(glm::vec3{ 0, 0, 2.0f }, glm::vec3{ 0, 0, 0 }, glm::vec3{ 0, 0, 1.0f });
			defaultView.projMat = glm::perspective(glm::radians(60.0f),
												   static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight),
												   defaultView.nearClipPlane, defaultView.farClipPlane);
												   //glm::ortho(0.0f, static_cast<float>(viewportWidth), static_cast<float>(viewportHeight), 0.0f, -1.0f, 1.0f);
			defaultView.viewProjMat = defaultView.projMat * glm::mat4(defaultView.viewMat);

			m_viewports[0].left = 0;
			m_viewports[0].top = 0;
			m_viewports[0].width = viewportWidth;
			m_viewports[0].height = viewportHeight;
			setViewportParameters(0, std::move(defaultView));

			// TEMP create some test resources
			try {
				//auto mdl = loadModel(L"models/Spitfire/spitfire.gmd", CacheType::Cache_Models);
				auto mdl = loadModel(L"models/landing_platform.gmd", CacheType::Cache_Models);
				//auto mdl = loadModel(L"models/collision_test/collision_test.gmd", CacheType::Cache_Models);
				//auto mdl = loadModel(L"models/gunship/gunship.gmd", CacheType::Cache_Models);
				//auto mdl = loadModel(L"models/A-10C Pit.gmd", CacheType::Cache_Models);
				//auto mdl = loadModel(L"models/other_pit.gmd", CacheType::Cache_Models);
				//auto mdl = loadModel(L"models/Bill/Bill.gmd", CacheType::Cache_Models);
				//auto mdl = loadModel(L"models/ring/ring.gmd", CacheType::Cache_Models);
				//auto mdl = loadModel(L"models/building_001.gmd", CacheType::Cache_Models);

				using namespace resource;
				auto loader = g_resourceLoader.lock();
				if (!loader) {
					throw std::runtime_error("no resource loader");
				}
				g_tempModel = loader->getResource(mdl).get();
				loader->executeCallbacks();
			}
			catch (std::exception ex) {
				SDL_Log("%s", ex.what());
			}
		}


		void RenderSystem::renderFrame(float interpolation)
		{
			for (int v = 0; v < MAX_VIEWPORTS; ++v) {
				Viewport& viewport = m_viewports[v];
				if (viewport.display) {
					viewport.renderQueue.sortRenderQueue();

					if (viewport.rendererType == RendererType_Deferred) {
						m_deferredRenderer.renderViewport(viewport.params);
					}

					viewport.renderQueue.clearRenderEntries();
				}
			}
		}


		RenderSystem::~RenderSystem()
		{
			if (g_glQuadVAO != 0) {
				glDeleteVertexArrays(1, &g_glQuadVAO);
			}
			if (g_glCubeVAO != 0) {
				glDeleteVertexArrays(1, &g_glCubeVAO);
			}
		}


		// Free Functions, RenderResources.h

		ResourceHandle<Texture2D_GL> loadTexture2D(wstring texturePath, CacheType cache)
		{
			using namespace resource;

			auto loader = g_resourceLoader.lock();

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


		ResourceHandle<TextureCubeMap_GL> loadTextureCubeMap(wstring texturePath, CacheType cache, bool swapY)
		{
			using namespace resource;

			auto loader = g_resourceLoader.lock();

			if (!loader) {
				throw std::runtime_error("no resource loader");
			}

			auto textureResourceBuilder = [](DataPtr data, size_t size) {
				TextureCubeMap_GL tex(move(data), size);
				SDL_Log("building texture of size %d", size);
				return tex;
			};

			// need a way to specify thread affinity for the callback so it knows to run on update or render thread
			auto textureResourceCallback = [](const ResourcePtr& resourcePtr, Id_T handle, size_t size) {
				TextureCubeMap_GL& tex = resourcePtr->getResource<TextureCubeMap_GL>();
				SDL_Log("callback texture of size %d", size);
				// the unique_ptr of data is stored within the texture, this call deletes the data after
				// sending texture to OpenGL
				tex.loadFromInternalMemory(true, true);
			};

			return loader->load<TextureCubeMap_GL>(texturePath, cache, textureResourceBuilder, textureResourceCallback);
		}


		ResourceHandle<ShaderProgram_GL> loadShaderProgram(wstring programPath, CacheType cache)
		{
			using namespace resource;

			auto loader = g_resourceLoader.lock();

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


		ResourceHandle<Model_GL> loadModel(wstring modelFilePath, CacheType cache)
		{
			using namespace resource;

			auto loader = g_resourceLoader.lock();

			if (!loader) {
				throw std::runtime_error("no resource loader");
			}

			auto meshResourceBuilder = [](DataPtr data, size_t size) {
				return Model_GL(Mesh_GL(std::move(data), size));
			};

			auto meshResourceCallback = [modelFilePath](const ResourcePtr& resourcePtr, Id_T handle, size_t size) {
				Model_GL& mdl = resourcePtr->getResource<Model_GL>();
				mdl.m_mesh.createResourcesFromInternalMemory(modelFilePath);
			};

			return loader->load<Model_GL>(modelFilePath, cache, meshResourceBuilder, meshResourceCallback);
		}

	}
}