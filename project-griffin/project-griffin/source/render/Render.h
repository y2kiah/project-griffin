#pragma once
#ifndef GRIFFIN_RENDER_H_
#define GRIFFIN_RENDER_H_

#include <string>
#include <memory>
#include "VertexBuffer_GL.h"
#include "RenderTarget_GL.h"
#include <utility/memory_reserve.h>
#include <resource/Resource.h>
#include <render/texture/Texture2D_GL.h>
#include <render/ShaderProgram_GL.h>
#include <render/Camera.h>

namespace griffin {
	// Forward Declarations
	namespace resource { class ResourceLoader; }

	namespace render {

		using std::unique_ptr;
		using std::weak_ptr;
		using std::shared_ptr;
		using std::wstring;
		using resource::ResourceHandle;
		using resource::ResourcePtr;
		using resource::CacheType;

		// Variables
		extern weak_ptr<resource::ResourceLoader> g_loaderPtr;

		/**
		* @struct RenderQueueKey
		* @var	depth		depth value converted to 16 bit integer
		* @var	material	internal material feature bits, determines shader
		* @var	instanced	0=no, 1=yes mesh is instanced
		* @var	translucencyType	0=opaque, 1=back-to-front translucent, 2=additive, 3=subtractive
		* @var	viewportLayer	0=skybox, 1=scene geometry, etc.
		* @var	viewport	viewport index to render
		* @var	fullscreenLayer	0=light pre-pass, 1=scene, 2=post filter, 3=HUD, 4=UI
		* @var	value		unioned with the above four vars, used for sorting
		*/
		struct RenderQueueKey {
			union {
				struct OpaqueKey {
					uint32_t frontToBackDepth;
					uint16_t material;

					uint16_t instanced        : 1;
					uint16_t translucencyType : 2;
					uint16_t viewportLayer    : 4;
					uint16_t viewport         : 5;
					uint16_t fullscreenLayer  : 4;
				};
				struct TranslucentKey {
					uint64_t backToFrontDepth : 48;
					uint64_t translucentPad   : 16;
				};
				uint64_t value;
			};
		};


		struct RenderEntry {
			glm::mat4	modelToWorld;

		};


		class RenderQueue {
		public:
			typedef struct { RenderQueueKey key; int entryIndex; } KeyType;
			typedef std::vector<KeyType>		KeyList;
			typedef std::vector<RenderEntry>	EntryList;

			explicit RenderQueue() {
				m_keys.reserve(RESERVE_RENDER_QUEUE);
				m_entries.reserve(RESERVE_RENDER_QUEUE);
			}

			~RenderQueue();

			void addRenderEntry(RenderQueueKey sortKey, RenderEntry&& entry);
			
			/**
			* Sorts the keys, will be the traversal order for rendering
			*/
			void sortRenderQueue();

			/**
			* Clear the keys and entries lists each frame
			*/
			void clearRenderEntries() {
				m_keys.clear();
				m_entries.clear();
			}

		private:
			KeyList		m_keys;
			EntryList	m_entries;
		};


		class DeferredRenderer_GL {
		public:
			explicit DeferredRenderer_GL() :
				m_gbuffer(RenderTarget_GL::GBuffer),
				m_colorBuffer(RenderTarget_GL::Color)
			{}
			~DeferredRenderer_GL();

			/**
			* Initialize the renderer
			*/
			void init(int viewportWidth, int viewportHeight);

			void renderFrame(double interpolation);

			void drawFullscreenQuad(/*Viewport?*/) const;

		private:
			RenderTarget_GL		m_gbuffer;							//<! g-buffer for deferred rendering
			
			uint32_t			m_glQuadVAO = 0;					//<! Vertex Array Object for fullScreenQuad
			VertexBuffer_GL		m_fullScreenQuad;

			ResourcePtr			m_mrtProgram = nullptr;				//<! multiple render target geometry pass, renders the g-buffer
			ResourcePtr			m_fullScreenQuadProgram = nullptr;	//<! fullscreen quad program for deferred lighting and post-processing
			ResourcePtr			m_ssaoProgram = nullptr;			//<! post-process screen space ambient occlusion shader
			ResourcePtr			m_atmosphereProgram = nullptr;		//<! post-process atmospheric scattering shader
			ResourcePtr			m_fxaaProgram = nullptr;			//<! post-process FXAA shader

			ResourcePtr			m_normalsTexture = nullptr;			//<! random normal noise texture for ssao

			RenderTarget_GL		m_colorBuffer;						//<! color buffer for FXAA
		};


		// Resource Loading Functions - do these belong here?
		ResourceHandle<Texture2D_GL>     loadTexture(wstring texturePath, CacheType cache = CacheType::Cache_Materials_T);
		ResourceHandle<ShaderProgram_GL> loadShaderProgram(wstring programPath, CacheType cache = CacheType::Cache_Materials_T);

		/**
		* need the systems to
		*	- get full render states 1 and 2 from entity store, each holds copy of components
		*		- if object in state 1 is missing from state 2, or vice versa, don't render it
		*		- for each component in both states, interpolate between state 1 and state 2
		*			- update position to interpolated value in quadtree culling system
		*		- for each viewport, active camera's frustum sent to culling system
		*			- frustum culled against quad tree objects, sends back list of rendered objects
		*			- render called on each object, submits RenderEntry to render system
		*	
		*	- update on fixed timesteps
		*		- scene graph transforms updated, with dirty flag optimization
		*		- when new scene info is ready, queue an event to tell render thread to swap states
		*/
		class RenderSystem {
		public:
			explicit RenderSystem() :
				m_cameras(0, RESERVE_RENDER_CAMERAS)
			{}

			~RenderSystem();

			void init(int viewportWidth, int viewportHeight);

			// needed?
			void interpolateStates(double interpolation) {}

			void renderFrame(double interpolation) {
				m_renderer.renderFrame(interpolation);
			}

		private:
			handle_map<unique_ptr<Camera>>  m_cameras;

			RenderQueue         m_renderQueue;
			DeferredRenderer_GL m_renderer;
		};


		typedef shared_ptr<RenderSystem> RenderSystemPtr;
	}
}

#endif