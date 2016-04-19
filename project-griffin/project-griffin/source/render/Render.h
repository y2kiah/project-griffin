#pragma once
#ifndef GRIFFIN_RENDER_H_
#define GRIFFIN_RENDER_H_

#pragma warning(disable:4003)

#include <string>
#include <memory>
#include <functional>
#include <bitset>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <utility/memory_reserve.h>
#include <utility/enum.h>
#include <utility/container/handle_map.h>
#include "RenderTarget_GL.h"

using std::unique_ptr;
using std::weak_ptr;
using std::shared_ptr;
using std::wstring;

// Forward Declarations
typedef struct NVGcontext NVGcontext;

namespace griffin {
	struct Engine;

	namespace resource {
		class ResourceLoader;
		typedef shared_ptr<ResourceLoader>	ResourceLoaderPtr;
		typedef weak_ptr<ResourceLoader>	ResourceLoaderWeakPtr;
		class Resource_T;
		typedef shared_ptr<Resource_T>	ResourcePtr;
	}

	namespace render {
		using resource::ResourcePtr;

		// Enums

		/**
		* Global fonts loaded at startup, in memory for the lifetime of the render system.
		* Pass these values to getFontId() function to get a font handle for the vector renderer.
		*/
		MakeEnum(FontFace, uint8_t,
			(Sans)
			(SansBold)
			(SansItalic)
			(SansBoldItalic)
			(Exo)
			(ExoBold)
			(ExoItalic)
			(ExoBoldItalic)
			,);

		/**
		* Layer of geometry for the scene render passes, specified in render queue key. Value is
		* ignored for ScenePostPass and FinalPostPass layers.
		*/
		MakeEnum(RenderQueueSceneLayer, uint8_t,
			(SceneLayer_SceneGeometry)			//<! scene geometry for the g-buffer pass
			(SceneLayer_LightVolumeGeometry)	//<! light volume geometry for deferred renderer
			(SceneLayer_Skybox)					//<! skybox is a special case handled in the deferred renderer
			(SceneLayer_Translucent)			//<! translucent geometry is rendered after framebuffer composition
			(SceneLayer_VectorGeometry)			//<! uses the vector renderer to overlay into the scene
			,);

		/**
		* Fullscreen layer determines order of composition. Each layer is effectively its own
		* scene with unique geometry. The ScenePostPass and FinalPostPass layers are special cases
		* where custom fullscreen filters can be added to the main scene, and final framebuffer.
		*/
		MakeEnum(RenderQueueFullscreenLayer, uint8_t,
			(FullscreenLayer_Scene)
			(FullscreenLayer_ScenePostPass)
			(FullscreenLayer_HUD)
			(FullscreenLayer_UI)
			(FullscreenLayer_Debug)
			(FullscreenLayer_FinalPostPass)
			,);

		MakeEnum(RenderQueueTranslucencyType, uint8_t,
			(TranslucencyType_AlphaTest)		//<! alpha test only no blend
			(TranslucencyType_AlphaBlend)		//<! using typical src_alpha, one_minus_src_alpha blend
			,);

		// Variables
		
		extern resource::ResourceLoaderWeakPtr g_resourceLoader;

		extern int g_fonts[FontFaceCount];


		// Functions

		void setResourceLoaderPtr(const resource::ResourceLoaderPtr& resourcePtr);

		/**
		* Helper to get a font face id
		*/
		inline int getFontId(FontFace face) { return g_fonts[face]; }


		// Types

		/**
		* @struct RenderQueueKey
		* @var	frontToBackDepth	depth value converted to 32 bit integer, used for opaque objects
		* @var	backToFrontDepth	inverse depth used for translucent objects
		* @var	material			internal material feature bits, determines shader
		* @var	instanced			0=no, 1=yes mesh is instanced
		* @var	translucencyType	one of the RenderQueueTranslucencyType values
		* @var	sceneLayer			one of the RenderQueueSceneLayer values
		* @var	fullscreenLayer		one of the RenderQueueFullscreenLayer values
		* @var	value				unioned with the vars above, used for sorting
		*/
		struct RenderQueueKey {
			union {
				struct AllKeys {			// AllKeys set for both opaque and translucent
					uint32_t _all_pad1;
					uint16_t _all_pad2;

					uint16_t instanced        : 1;
					uint16_t sceneLayer       : 4;
					uint16_t fullscreenLayer  : 4;
					uint16_t _reserved        : 7;
				};

				struct OpaqueKey {			// OpaqueKey 48 bits split between material and depth
					uint32_t frontToBackDepth;
					uint16_t material;

					uint16_t _opaque_pad;
				};

				struct TranslucentKey {		// TranslucentKey 46 bits for depth
					uint64_t translucencyType : 2;
					uint64_t backToFrontDepth : 46;

					uint64_t _translucent_pad : 16;
				};

				AllKeys        allKeys;
				OpaqueKey      opaqueKey;
				TranslucentKey translucentKey;

				uint64_t       value;
			};
		};


		struct RenderEntry {
			/**
			* @param Id_T	entity id
			* @param int	drawset/submesh index to render
			*/
			typedef std::function<void(Id_T, int)>	DrawCallback;

			Id_T			entityId;
			uint32_t		drawsetIndex;
			uint32_t		nodeIndex;
			uint32_t		parentNodeIndex;

			uint32_t		_padding_0;

			glm::dvec4		positionWorld;
			glm::dquat		orientationWorld;
			glm::dvec3		scale;
			
			DrawCallback	drawCallback;
			// render flags?
		};


		class RenderQueue {
		public:
			typedef struct {
				RenderQueueKey	key;
				uint32_t		entryIndex;
				uint32_t		_pad_end;
			} KeyType;
			typedef std::vector<KeyType>		KeyList;
			typedef std::vector<RenderEntry>	EntryList;

			explicit RenderQueue() {
				keys.reserve(RESERVE_RENDER_QUEUE);
				filteredKeys.reserve(RESERVE_RENDER_QUEUE);
				entries.reserve(RESERVE_RENDER_QUEUE);
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
				keys.clear();
				entries.clear();
			}

			KeyList		keys;			//<! set of keys to be sorted for rendering order
			KeyList		filteredKeys;	//<! filtered set of keys used internally by render system
			EntryList	entries;		//<! list of render entries for a frame
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
			std::bitset<RenderQueueFullscreenLayerCount> fullscreenLayers = 0; //<! bits indicating which fullscreen layers are rendered in the viewport
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

			/**
			* Renders a viewport's render queue keys.
			* @var viewport	the viewport to render
			* @var keys		list of keys to render, usually set to viewport.filteredKeys (after
			*				filtering) or viewport.keys for all, but any KeyList can be passed
			*/
			void renderGBuffer(Viewport& viewport, const RenderQueue::KeyList& keys, Engine& engine);
			void renderLightVolumes(Viewport& viewport, const RenderQueue::KeyList& keys, Engine& engine);
			void renderSkybox(Viewport& viewport, const RenderQueue::KeyList& keys, Engine& engine);
			void renderPostProcess(Viewport& viewport, const RenderQueue::KeyList& keys, Engine& engine);

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

			/**
			* Renders a viewport's render queue keys.
			* @var viewport	the viewport to render
			* @var keys		list of keys to render, usually set to viewport.filteredKeys (after
			*				filtering) or viewport.keys for all, but any KeyList can be passed
			*/
			void renderViewport(Viewport& viewport, const RenderQueue::KeyList& keys, Engine& engine);

			static void loadGlobalFonts(VectorRenderer_GL& inst);

		private:
			NVGcontext *	m_nvg = nullptr;
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
			* Separate call to initialize fonts since they are global state. Must be called after
			* init, uses the vector renderer instance.
			*/
			void loadGlobalFonts() {
				VectorRenderer_GL::loadGlobalFonts(m_vectorRenderer);
			}

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
			* Adds render entries into the viewport's render queue.
			*/
			void addRenderEntries(uint8_t viewport, const std::vector<RenderQueue::KeyType>& keys, const std::vector<RenderEntry>& entries)
			{
				assert(viewport < MAX_VIEWPORTS && "viewport out of range");
				assert(keys.size() == entries.size() && "keys and entries sizes must match");

				RenderQueue& queue = m_viewports[viewport].renderQueue;

				queue.keys.reserve(queue.keys.size() + keys.size());
				queue.entries.reserve(queue.entries.size() + entries.size());

				int startingSize = static_cast<int>(queue.keys.size());
				int size = static_cast<int>(keys.size());

				for (int k = 0; k < size; ++k) {
					queue.keys.push_back(keys[k]);
					queue.keys.back().entryIndex += startingSize;
				}

				for (int e = 0; e < size; ++e) {
					queue.entries.push_back(entries[e]);
				}
			}

			void renderFrame(float interpolation, Engine& engine);

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