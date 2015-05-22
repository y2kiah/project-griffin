#pragma once
#ifndef GRIFFIN_RENDER_H_
#define GRIFFIN_RENDER_H_

#include <string>
#include <memory>
#include "VertexBuffer_GL.h"
#include "RenderTarget_GL.h"
#include <utility/memory_reserve.h>

namespace griffin {
	// Forward Declarations
	namespace resource { class ResourceLoader; }

	namespace render {

		using std::unique_ptr;
		using std::weak_ptr;
		using std::wstring;

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
				m_gbuffer(RenderTarget_GL::GBuffer)
			{}

			/**
			* Initialize the renderer
			*/
			bool init(int viewportWidth, int viewportHeight);

		private:
			VertexBuffer_GL		m_fullScreenQuad;
			RenderTarget_GL		m_gbuffer;
//			ShaderProgram_GL	m_fullScreenQuadProgram;

		};

		// Functions
		void initRenderData(int viewportWidth, int viewportHeight);
		void renderFrame(double interpolation);

	}
}

#endif