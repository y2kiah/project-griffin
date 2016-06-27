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

#include <application/Engine.h>
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

		resource::ResourceLoaderWeakPtr g_resourceLoader;

		// TEMP
		ResourcePtr		g_tempModel[4] = {};

		// defined in RenderHelpers.cpp
		extern float			g_fullScreenQuadBufferData[3 * 6];
		extern uint32_t			g_glQuadVAO;
		extern VertexBuffer_GL	g_fullScreenQuad;
		extern float			g_cubeBufferData[3 * 36];
		extern uint32_t			g_glCubeVAO;
		extern VertexBuffer_GL	g_cubeBuffer;


		// Free functions

		void setResourceLoaderPtr(const resource::ResourceLoaderPtr& resourcePtr)
		{
			g_resourceLoader = resourcePtr;
		}


		// class RenderQueue

		void RenderQueue::addRenderEntry(RenderQueueKey sortKey, RenderEntry&& entry)
		{
			entries.push_back(std::forward<RenderEntry>(entry));
			keys.push_back({ sortKey, static_cast<int>(entries.size()) });
		}

		void RenderQueue::sortRenderQueue()
		{
			std::sort(keys.begin(), keys.end(), [](const KeyType& a, const KeyType& b) {
				return (a.key.value < b.key.value);
			});
		}

		RenderQueue::~RenderQueue()
		{
			if (keys.capacity() > RESERVE_RENDER_QUEUE) {
				SDL_Log("check RESERVE_RENDER_QUEUE: original=%d, highest=%d", RESERVE_RENDER_QUEUE, keys.capacity());
			}
		}


		// class DeferredRenderer_GL

		void DeferredRenderer_GL::init(int viewportWidth, int viewportHeight, Engine& engine)
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
			m_colorBuffer.bind(RenderTarget_GL::Albedo_Displacement, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // fxaa sampler requires bilinear filtering
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			// load shader programs for deferred rendering
			// hold a shared_ptr to these shader programs so they never fall out of cache

			//auto fsq  = loadShaderProgram(L"shaders/fullscreenQuad.glsl");
			auto mrt  = loadShaderProgram(L"shaders/ads.glsl", engine.renderSystem); // temporarily ads.glsl
			auto sky  = loadShaderProgram(L"shaders/skybox.glsl", engine.renderSystem);
			auto ssao = loadShaderProgram(L"shaders/ssao.glsl", engine.renderSystem);
			auto fxaa = loadShaderProgram(L"shaders/fxaa.glsl", engine.renderSystem);
			//L"shaders/linearDepth.glsl"
			//L"shaders/SimpleShader.glsl"

			auto nrml = loadTexture2D(L"textures/normal-noise.dds", CacheType::Cache_Permanent);

			//m_fullScreenQuadProgram = loader->getResource(fsq).get(); // wait on the futures and assign shared_ptrs
			m_mrtProgram = loader->getResource(mrt).get();
			m_skyboxProgram = loader->getResource(sky).get();
			m_ssaoProgram = loader->getResource(ssao).get();
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

			assert(glGetError() == GL_NO_ERROR);
		}


		// class DeferredRenderer_GL

		void DeferredRenderer_GL::renderGBuffer(Viewport& viewport, const RenderQueue::KeyList& keys, Engine& engine)
		{
			auto& renderSystem = *engine.renderSystem;
			static float animTime = 0.0f; // TEMP

			// set the camera UBO
			CameraUniformsUBO cameraUniformsUBO = {
				viewport.params.projMat,
				viewport.params.viewProjMat,
				viewport.params.nearClipPlane,
				viewport.params.farClipPlane,
				viewport.params.inverseFrustumDistance
			};
			glBindBuffer(GL_UNIFORM_BUFFER, renderSystem.getUBOHandle(CameraUniforms));
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CameraUniformsUBO), &cameraUniformsUBO);

			// Start g-buffer rendering
			m_gbuffer.start();
			{
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
				
				glEnable(GL_DEPTH_TEST);
				
				glEnable(GL_STENCIL_TEST);
				glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
				glStencilFunc(GL_ALWAYS, 1, 0xFF);
				glStencilMask(0xFF);

				auto& program = m_mrtProgram.get()->getResource<ShaderProgram_GL>();
				program.useProgram();
				auto programId = program.getProgramId();

				// TEMP get material uniform locations
				GLint ambientLoc = glGetUniformLocation(programId, "material.Ma");
				GLint diffuseLoc = glGetUniformLocation(programId, "material.Md");
				GLint specularLoc = glGetUniformLocation(programId, "material.Ms");
				GLint emissiveLoc = glGetUniformLocation(programId, "material.Me");
				GLint shininessLoc = glGetUniformLocation(programId, "material.shininess");
				GLint metallicLoc = glGetUniformLocation(programId, "material.metallic");

				GLint diffuseMapLoc = glGetUniformLocation(programId, "diffuseMap");

				// TEMP set light uniforms
				// TODO, this is not relevant in g-buffer render step, move to lightVolume render
				GLint lightPosLoc = glGetUniformLocation(programId, "light.positionViewspace");
				GLint lightDirLoc = glGetUniformLocation(programId, "light.directionViewspace");
				GLint lightLaLoc = glGetUniformLocation(programId, "light.La");
				GLint lightLdsLoc = glGetUniformLocation(programId, "light.Lds");
				GLint lightKcLoc = glGetUniformLocation(programId, "light.Kc");
				GLint lightKlLoc = glGetUniformLocation(programId, "light.Kl");
				GLint lightKqLoc = glGetUniformLocation(programId, "light.Kq");
				GLint lightAngleLoc = glGetUniformLocation(programId, "light.spotAngleCutoff");
				GLint lightEdgeLoc = glGetUniformLocation(programId, "light.spotEdgeBlendPct");

				glm::vec4 lightPos{ 0.1f, 1.0f, 150.0f, 1.0f };
				glm::vec4 lightDir{ 0.0f, 0.0f, -1.0f, 0.0f };
				glm::vec4 lightPosViewspace = viewport.params.viewMat * lightPos;
				glm::vec3 lightDirViewspace = glm::normalize(glm::vec3(viewport.params.viewMat * lightDir));
				//glm::vec4 lightPosViewspace{ 0.0f, 0.0f, 0.0f, 1.0f };
				//glm::vec3 lightDirViewspace{ 0.0f, 0.0f, -1.0f };
				glm::vec3 lightLa{ 0.8f, 0.9f, 1.0f };
				glm::vec3 lightLds{ 0.8f, 0.6f, 0.3f };
				glUniform4fv(lightPosLoc, 1, &lightPosViewspace[0]);
				glUniform3fv(lightDirLoc, 1, &lightDirViewspace[0]);
				glUniform3fv(lightLaLoc, 1, &lightLa[0]);
				glUniform3fv(lightLdsLoc, 1, &lightLds[0]);
				glUniform1f(lightKcLoc, 1.0f);
				glUniform1f(lightKlLoc, 0.007f);
				glUniform1f(lightKqLoc, 0.0002f);
				glUniform1f(lightAngleLoc, 0.96f);
				glUniform1f(lightEdgeLoc, 0.4f);

				// TEMP set viewport uniforms
				/*GLint viewProjMatLoc = glGetUniformLocation(programId, "viewProjection");
				GLint frustumNearLoc = glGetUniformLocation(programId, "frustumNear");
				GLint frustumFarLoc = glGetUniformLocation(programId, "frustumFar");
				GLint inverseFrustumDistanceLoc = glGetUniformLocation(programId, "inverseFrustumDistance");
				glUniformMatrix4fv(viewProjMatLoc, 1, GL_FALSE, &viewport.params.viewProjMat[0][0]);
				glUniform1f(frustumNearLoc, viewport.params.nearClipPlane);
				glUniform1f(frustumFarLoc, viewport.params.farClipPlane);
				glUniform1f(inverseFrustumDistanceLoc, viewport.params.inverseFrustumDistance);
				*/

				// render all keys
				int lastDrawsetIndex = -1;
				for (auto key : keys) {
					auto& entry = viewport.renderQueue.entries[key.entryIndex];

					entry.drawCallback(entry.entityId, entry.drawsetIndex);
				}

				// TEMP
				animTime += 0.001667f;
				if (animTime > 2.5f) {
					animTime = 0.0f;
				}

				// draw the test mesh
				for (int i = 0; i < _countof(g_tempModel); ++i) {
					if (g_tempModel[i]) {
						auto& mdl = g_tempModel[i]->getResource<Model_GL>();
						mdl.m_mesh.render(engine, 0,
										  programId,
										  ambientLoc, diffuseLoc, specularLoc, shininessLoc,
										  diffuseMapLoc, animTime,
										  viewport.params.viewMat, viewport.params.projMat); // temporarily passing in the modelMatLoc
					}
				}

				glDisable(GL_DEPTH_TEST);
				
				// Render Skybox
				if (m_skyboxTexture) {
					auto& program = m_skyboxProgram.get()->getResource<ShaderProgram_GL>();
					program.useProgram();
					auto programId = program.getProgramId();

					auto& skybox = m_skyboxTexture.get()->getResource<TextureCubeMap_GL>();
					skybox.bind();

					GLint mvpMatLoc  = glGetUniformLocation(programId, "modelViewProjection");
					GLint cubemapLoc = glGetUniformLocation(programId, "cubemap");

					glm::mat4 skyboxViewMat = viewport.params.viewMat;
					skyboxViewMat[3].xyz = 0.0f;
					glm::mat4 skyboxMVP = viewport.params.projMat * skyboxViewMat;

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

				m_normalsTexture.get()->getResource<Texture2D_GL>().bind(0);
				m_gbuffer.bind(RenderTarget_GL::Albedo_Displacement, 1);
				m_gbuffer.bind(RenderTarget_GL::Normal_Reflectance, 2);
				m_gbuffer.bind(RenderTarget_GL::Depth_Stencil, 3);
				m_gbuffer.bind(RenderTarget_GL::Position, 4);

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
				
				glUniform1f(cameraNearLoc, viewport.params.nearClipPlane);
				glUniform1f(cameraFarLoc, viewport.params.farClipPlane);

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
				m_colorBuffer.bind(RenderTarget_GL::Albedo_Displacement, 0);	// when SSAO on
				
				//m_gbuffer.bind(RenderTarget_GL::Albedo_Displacement, 0);		// when SSAO off, alpha channel must contain luma
				//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				GLint colorMapLoc = glGetUniformLocation(fxaaId, "colorMap");
				glUniform1i(colorMapLoc, 0);

				drawFullscreenQuad();
			}
			// End post-processing

			assert(glGetError() == GL_NO_ERROR);
		}

		DeferredRenderer_GL::~DeferredRenderer_GL()
		{
		}


		// class RenderSystem

		void RenderSystem::init(int viewportWidth, int viewportHeight, Engine& engine) {
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
			m_deferredRenderer.init(viewportWidth, viewportHeight, engine);
			m_vectorRenderer.init(viewportWidth, viewportHeight);

			// set up default viewport matrices
			ViewParameters defaultView{};
			defaultView.nearClipPlane = 0.1f; //-1.0f;
			defaultView.farClipPlane  = 100000.0f; //1.0f;
			defaultView.frustumDistance = defaultView.farClipPlane - defaultView.nearClipPlane;
			defaultView.inverseFrustumDistance = 1.0f / defaultView.frustumDistance;
			defaultView.viewMat = glm::lookAt(glm::dvec3{ 120.0, 40.0, 0.0 }, glm::dvec3{ 0.0, 0.0, 0.0 }, glm::dvec3{ 0.0, 0.0, 1.0 });
								  //glm::lookAt(glm::dvec3{ 0.0, 0.0, 2.0 }, glm::dvec3{ 0.0, 0.0, 0.0 }, glm::dvec3{ 0.0, 0.0, 1.0 });
			defaultView.projMat = glm::perspective(glm::radians(60.0f),
												   static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight),
												   defaultView.nearClipPlane, defaultView.farClipPlane);
												   //glm::ortho(0.0f, static_cast<float>(viewportWidth), static_cast<float>(viewportHeight), 0.0f, -1.0f, 1.0f);
			defaultView.viewProjMat = defaultView.projMat * glm::mat4(defaultView.viewMat);

			m_viewports[0].left = 0;
			m_viewports[0].top = 0;
			m_viewports[0].width = viewportWidth;
			m_viewports[0].height = viewportHeight;
			m_viewports[0].params = defaultView;
			m_viewports[0].display = true;

			ViewParameters guiView{};
			guiView.nearClipPlane = -1.0f;
			guiView.farClipPlane = 1.0f;
			guiView.frustumDistance = guiView.farClipPlane - guiView.nearClipPlane;
			guiView.inverseFrustumDistance = 1.0f / guiView.frustumDistance;
			guiView.viewMat = glm::lookAt(glm::dvec3{ 0.0, 0.0, 0.0 }, glm::dvec3{ 0.0, 0.0, 0.0 }, glm::dvec3{ 0.0, 0.0, 1.0 });
			guiView.projMat = glm::ortho(0.0f, static_cast<float>(viewportWidth), static_cast<float>(viewportHeight), 0.0f,
										 guiView.nearClipPlane, guiView.farClipPlane);
			guiView.viewProjMat = guiView.projMat * glm::mat4(guiView.viewMat);

			m_viewports[1].left = 0;
			m_viewports[1].top = 0;
			m_viewports[1].width = viewportWidth;
			m_viewports[1].height = viewportHeight;
			m_viewports[1].params = guiView;
			m_viewports[1].display = true;

			// Create the UBOs
			glGenBuffers(UBOTypeCount, m_uboHandles);

			glBindBuffer(GL_UNIFORM_BUFFER, m_uboHandles[CameraUniforms]);
			glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraUniformsUBO), nullptr, GL_DYNAMIC_DRAW);

			glBindBuffer(GL_UNIFORM_BUFFER, m_uboHandles[ObjectUniforms]);
			glBufferData(GL_UNIFORM_BUFFER, sizeof(ObjectUniformsUBO), nullptr, GL_DYNAMIC_DRAW);

			// TEMP create some test resources
			try {
				//auto mdl1 = loadModel(L"models/Spitfire/spitfire.gmd", CacheType::Cache_Models);
				auto mdl2 = loadModel(L"models/landing_platform.gmd", CacheType::Cache_Models);
				//auto mdl3 = loadModel(L"models/collision_test/collision_test.gmd", CacheType::Cache_Models);
				//auto mdl4 = loadModel(L"models/gunship/gunship.gmd", CacheType::Cache_Models);
				//auto mdl5 = loadModel(L"models/A-10C Pit.gmd", CacheType::Cache_Models);
				//auto mdl6 = loadModel(L"models/other_pit.gmd", CacheType::Cache_Models);
				//auto mdl7 = loadModel(L"models/Bill/Bill.gmd", CacheType::Cache_Models);
				//auto mdl8 = loadModel(L"models/ring/ring.gmd", CacheType::Cache_Models);
				//auto mdl9 = loadModel(L"models/building_001.gmd", CacheType::Cache_Models);
				//auto mdl10 = loadModel(L"models/scene/scene.gmd", CacheType::Cache_Models);

				using namespace resource;
				auto loader = g_resourceLoader.lock();
				if (!loader) {
					throw std::runtime_error("no resource loader");
				}
				g_tempModel[0] = loader->getResource(mdl2).get();
				//g_tempModel[1] = loader->getResource(mdl4).get();
				//g_tempModel[2] = loader->getResource(mdl5).get();
				//g_tempModel[3] = loader->getResource(mdl8).get();
				loader->executeCallbacks();
			}
			catch (std::exception ex) {
				SDL_Log("%s", ex.what());
			}

			assert(glGetError() == GL_NO_ERROR);
		}


		/**
		* Filters keys, beginning at startIndex, by comparing (filter & mask) to each (key & mask).
		*	If the resulting masked values match, the filter passes and the key is pushed to the
		*	back of filteredKeys.
		* @var startIndex	Start of key range to filter. If value passed is < 0, 0 will be used.
		* @var earlyExit	If true, exits on first failed key after first passing key is found.
		*					Set to true when you don't expect gaps in the passing set of keys.
		* @returns	One past the last passing key index, or value of startIndex if no keys found.
		*			The intention is to pass this value into startIndex of subsequent calls to
		*			progress through the list in a forward direction.
		*/
		int filterKeys(const RenderQueue::KeyList& sortedKeys, int startIndex,
					   RenderQueueKey filter, RenderQueueKey mask,
					   RenderQueue::KeyList& filteredKeys,
					   bool earlyExit = false)
		{
			if (startIndex < 0) { startIndex = 0; }
			int lastKey = startIndex; // last passing key

			for (int k = startIndex; k < sortedKeys.size(); ++k) {
				auto thisKey = sortedKeys[k];
				// mask only the bits that matter, and compare the values
				if ((thisKey.key.value & mask.value) == (filter.value & mask.value)) {
					filteredKeys.push_back(thisKey);
					lastKey = k + 1;
				}
				else if (earlyExit && lastKey != startIndex) {
					break;
				}
			}

			return lastKey;
		}


		void RenderSystem::renderFrame(float interpolation, Engine& engine)
		{
			using namespace resource;
			auto loader = g_resourceLoader.lock();
			if (!loader) {
				throw std::runtime_error("no resource loader");
			}

			// for each active viewport
			for (int v = 0; v < MAX_VIEWPORTS; ++v) {
				Viewport& viewport = m_viewports[v];
				
				#if 1
				// TEMP
				if (v == 0) {
					m_deferredRenderer.renderGBuffer(viewport, viewport.renderQueue.filteredKeys, engine);
					m_vectorRenderer.renderViewport(viewport, viewport.renderQueue.filteredKeys, engine);
				}
				#endif
				
				if (viewport.display) {
					int keyStart = 0; // start of key range for filtering entries
					
					viewport.renderQueue.sortRenderQueue();
					
					if (viewport.fullscreenLayers[FullscreenLayer_Scene]) {
						// render the main scene's g-buffer with deferred renderer
						RenderQueueKey filter{};
						filter.allKeys.fullscreenLayer = FullscreenLayer_Scene;
						filter.allKeys.sceneLayer = SceneLayer_SceneGeometry;
						RenderQueueKey mask{};
						mask.allKeys.fullscreenLayer = 0xF;
						mask.allKeys.sceneLayer = 0xF;

						// gather main scene entries for deferred renderer into filteredKeys set
						viewport.renderQueue.filteredKeys.clear();
						keyStart = filterKeys(viewport.renderQueue.keys, keyStart, filter, mask,
											  viewport.renderQueue.filteredKeys,
											  true); // no gaps in keys, so exit early is ok
						m_deferredRenderer.renderGBuffer(viewport, viewport.renderQueue.filteredKeys, engine);

						// render the main scene's lighting pass
						filter.allKeys.sceneLayer = SceneLayer_LightVolumeGeometry;
						viewport.renderQueue.filteredKeys.clear();
						keyStart = filterKeys(viewport.renderQueue.keys, keyStart, filter, mask, viewport.renderQueue.filteredKeys, true);
						//m_deferredRenderer.renderLightVolumes(viewport, viewport.renderQueue.filteredKeys);

						// render the main scene's skybox
						filter.allKeys.sceneLayer = SceneLayer_Skybox;
						viewport.renderQueue.filteredKeys.clear();
						keyStart = filterKeys(viewport.renderQueue.keys, keyStart, filter, mask, viewport.renderQueue.filteredKeys, true);
						//m_deferredRenderer.renderSkybox(viewport, viewport.renderQueue.filteredKeys);

						// render the main scene's translucent geometry
						filter.allKeys.sceneLayer = SceneLayer_Translucent;
						viewport.renderQueue.filteredKeys.clear();
						keyStart = filterKeys(viewport.renderQueue.keys, keyStart, filter, mask, viewport.renderQueue.filteredKeys, true);
						// forward render translucent stuff

						// render the main scene's vector geometry
						filter.allKeys.sceneLayer = SceneLayer_VectorGeometry;
						viewport.renderQueue.filteredKeys.clear();
						keyStart = filterKeys(viewport.renderQueue.keys, keyStart, filter, mask, viewport.renderQueue.filteredKeys, true);
						m_vectorRenderer.renderViewport(viewport, viewport.renderQueue.filteredKeys, engine);
					}

					viewport.renderQueue.clearRenderEntries();
				}
			}
		}


		RenderSystem::~RenderSystem()
		{
			if (m_uboHandles[0] != 0) {
				glDeleteBuffers(UBOTypeCount, m_uboHandles);
			}
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


		ResourceHandle<TextureCubeMap_GL> loadTextureCubeMap(wstring texturePath, bool swapY, CacheType cache)
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
			// TODO: switch to using task system, take advantage of thread affinity in that system
			auto textureResourceCallback = [](const ResourcePtr& resourcePtr, Id_T handle, size_t size) {
				TextureCubeMap_GL& tex = resourcePtr->getResource<TextureCubeMap_GL>();
				SDL_Log("callback texture of size %d", size);
				// the unique_ptr of data is stored within the texture, this call deletes the data after
				// sending texture to OpenGL
				tex.loadFromInternalMemory(true, true);
			};

			return loader->load<TextureCubeMap_GL>(texturePath, cache, textureResourceBuilder, textureResourceCallback);
		}


		ResourceHandle<ShaderProgram_GL> loadShaderProgram(wstring programPath, const RenderSystemPtr &renderSystemPtr, CacheType cache)
		{
			using namespace resource;

			auto loader = g_resourceLoader.lock();

			if (!loader) {
				throw std::runtime_error("no resource loader");
			}

			auto shaderResourceBuilder = [programPath](DataPtr data, size_t size) {
				string shaderCode(reinterpret_cast<char*>(data.get()), size);
				return ShaderProgram_GL(shaderCode, programPath);
			};

			auto shaderResourceCallback = [renderSystemPtr](const ResourcePtr& resourcePtr, Id_T handle, size_t size) {
				ShaderProgram_GL& program = resourcePtr->getResource<ShaderProgram_GL>();
				auto ok = program.compileAndLinkProgram();
				if (!ok) {
					SDL_LogError(SDL_LOG_CATEGORY_RENDER, "  program compilation/linking failed");
					throw std::runtime_error("program compilation/linking failed");
				}

				// bind render system's UBOs to the shader's uniform buffer blocks
				for (int uboType = 0; uboType < UBOTypeCount; ++uboType) {
					program.bindUniformBuffer(static_cast<UBOType>(uboType), renderSystemPtr->getUBOHandle(static_cast<UBOType>(uboType)));
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

			auto modelResourceBuilder = [/*modelFilePath*/](DataPtr data, size_t size) {
				Model_GL mdl(Mesh_GL(std::move(data), size));
				mdl.initRenderEntries();
				//mdl.loadMaterialResources(modelFilePath);
				return mdl;
			};

			auto modelResourceCallback = [modelFilePath](const ResourcePtr& resourcePtr, Id_T handle, size_t size) {
				Model_GL& mdl = resourcePtr->getResource<Model_GL>();
				mdl.createBuffers();
				mdl.loadMaterialResources(modelFilePath); // TEMP, switch to calling from the builder once internal texture loading is made async
			};

			return loader->load<Model_GL>(modelFilePath, cache, modelResourceBuilder, modelResourceCallback);
		}

	}
}