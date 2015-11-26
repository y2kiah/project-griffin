#pragma once
#ifndef GRIFFIN_RENDER_H_
#define GRIFFIN_RENDER_H_

#include <string>
#include <memory>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <resource/ResourceLoader.h>
#include <utility/memory_reserve.h>
#include "RenderTarget_GL.h"

// Forward Declarations
typedef struct NVGcontext NVGcontext;

namespace griffin {
	namespace resource { class ResourceLoader; }

	namespace render {
		using std::unique_ptr;
		using std::weak_ptr;
		using std::shared_ptr;
		using std::wstring;
		using resource::ResourcePtr;
		using resource::CacheType;

		// Variables
		extern weak_ptr<resource::ResourceLoader> g_resourceLoader;

		// Types

		enum RendererType {
			RendererType_Deferred = 0,
			RendererType_Forward = 1,
			RendererType_Vector = 2
		};

		/**
		* @struct RenderQueueKey
		* @var	frontToBackDepth	depth value converted to 32 bit integer
		* @var	material	internal material feature bits, determines shader
		* @var	instanced	0=no, 1=yes mesh is instanced
		* @var	translucencyType	0=opaque, 1=back-to-front translucent, 2=additive, 3=subtractive
		* @var	sceneLayer	0=skybox, 1=scene geometry, etc.
		* @var	fullscreenLayer	0=light pre-pass, 1=game scene, 2=post filter, 3=HUD, 4=UI, 5=debug
		* @var	value		unioned with the above four vars, used for sorting
		*/
		struct RenderQueueKey {
			union {
				struct AllKeys {			// AllKeys set for both opaque and translucent
					uint32_t _all_pad1;
					uint16_t _all_pad2;

					uint16_t instanced        : 1;
					uint16_t translucencyType : 2;
					uint16_t sceneLayer       : 4;
					uint16_t fullscreenLayer  : 4;
					uint16_t _reserved        : 5;
				};
				struct OpaqueKey {			// OpaqueKey 48 bits split between material and depth
					uint32_t frontToBackDepth;
					uint16_t material;

					uint16_t _opaque_pad;
				};
				struct TransparentKey {		// TransparentKey 48 bits for depth alone
					uint64_t backToFrontDepth : 48;

					uint64_t _translucent_pad : 16;
				};
				uint64_t value;
			};
		};


		struct RenderEntry {
			/**
			* @param Id_T	entity id
			* @param int	drawset/submesh index to render
			*/
			typedef std::function<void(Id_T, int)>	DrawCallback;

			glm::dvec3		positionWorld;
			glm::quat		orientationWorld;
			Id_T			entityId;
			DrawCallback	drawCallback;
			// render flags?
		};


		class RenderQueue {
		public:
			typedef struct {
				RenderQueueKey key;
				int entryIndex;
				int _pad_end;
			} KeyType;
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


		/**
		* Maximum number of render viewports. Multiple viewports can be used for multi-screen
		* rendering where each screen contains a separate camera view.
		*/
		#define MAX_VIEWPORTS	16

		struct ViewportParameters {
			glm::dmat4	viewMat;
			glm::mat4	projMat;
			glm::mat4	viewProjMat;
			float		nearClipPlane;
			float		farClipPlane;
			float		frustumDistance;		//<! = farClipPlane - nearClipPlane
			float		inverseFrustumDistance;	//<! = 1.0f / frustumDistance
		};


		struct Viewport {
			uint32_t			left;
			uint32_t			top;
			uint32_t			width;
			uint32_t			height;
			ViewportParameters	params;
			RenderQueue			renderQueue;
			bool				display = false;
			RendererType		rendererType = RendererType_Deferred;
			//RenderTarget ?? optional fbo, for rendering viewport to texture
		};


		/**
		*
		*/
		class DeferredRenderer_GL {
		public:
			explicit DeferredRenderer_GL() :
				m_gbuffer(RenderTarget_GL::TypeGBuffer),
				m_colorBuffer(RenderTarget_GL::TypeColor)
			{}
			~DeferredRenderer_GL();

			/**
			* Initialize the renderer
			*/
			void init(int viewportWidth, int viewportHeight);

			void renderViewport(Viewport& viewport);

			// temp, will be part of render queue in its own layer
			void setSkyboxTexture(const ResourcePtr& textureCubeMap) {
				m_skyboxTexture = textureCubeMap;
			}

		private:
			RenderTarget_GL		m_gbuffer;							//<! g-buffer for deferred rendering

			ResourcePtr			m_mrtProgram = nullptr;				//<! multiple render target geometry pass, renders the g-buffer
			ResourcePtr			m_skyboxProgram = nullptr;			//<! skybox render from cubemap texture
			ResourcePtr			m_fullScreenQuadProgram = nullptr;	//<! fullscreen quad program for deferred lighting and post-processing
			ResourcePtr			m_ssaoProgram = nullptr;			//<! post-process screen space ambient occlusion shader
			ResourcePtr			m_fxaaProgram = nullptr;			//<! post-process FXAA shader

			ResourcePtr			m_normalsTexture = nullptr;			//<! random normal noise texture for ssao
			
			// temp, will be part of render queue in its own layer
			ResourcePtr			m_skyboxTexture = nullptr;			//<! optional skybox cubemap passed in from external source

			RenderTarget_GL		m_colorBuffer;						//<! color buffer for FXAA
		};


		/**
		*
		*/
		class VectorRenderer_GL {
		public:
			explicit VectorRenderer_GL() {}
			~VectorRenderer_GL();

			/**
			* Initialize the renderer
			*/
			void init(int viewportWidth, int viewportHeight);

			void renderViewport(Viewport& viewport);

		private:
			NVGcontext *	m_nvg = nullptr;
			int				m_font = -1;
		};


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
			//typedef std::vector<Viewport>	ViewportList;

			explicit RenderSystem() {}

			~RenderSystem();

			void init(int viewportWidth, int viewportHeight);

			/**
			* Sets the camera matrices and clip planes. Needs to be set for each viewport rendered
			* and any time the view parameters change, usually once per frame.
			*/
			void setViewportParameters(uint8_t viewport, ViewportParameters&& viewportParams)
			{
				assert(viewport < MAX_VIEWPORTS && "viewport out of range");

				m_viewports[viewport].params = std::forward<ViewportParameters>(viewportParams);
				m_viewports[viewport].display = true;
			}

			/**
			* Adds a render entry into the viewport's render queue.
			*/
			void addRenderEntry(uint8_t viewport, RenderQueueKey sortKey, RenderEntry&& renderEntry)
			{
				assert(viewport < MAX_VIEWPORTS && "viewport out of range");

				m_viewports[viewport].renderQueue.addRenderEntry(sortKey, std::forward<RenderEntry>(renderEntry));
			}

			void renderFrame(float interpolation);

			// temp, will be part of render queue in its own layer
			void setSkyboxTexture(const ResourcePtr& textureCubeMap) {
				m_deferredRenderer.setSkyboxTexture(textureCubeMap);
			}

		private:
			Viewport			m_viewports[MAX_VIEWPORTS];
			DeferredRenderer_GL	m_deferredRenderer;
			VectorRenderer_GL	m_vectorRenderer;
		};


		typedef shared_ptr<RenderSystem> RenderSystemPtr;
		typedef weak_ptr<RenderSystem>   RenderSystemWeakPtr;

	}
}

#endif