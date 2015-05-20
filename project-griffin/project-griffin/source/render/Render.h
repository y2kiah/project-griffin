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
		* @var	material	
		* @var	depth		
		* @var	instance	
		* @var	translucencyType	
		* @var	viewportLayer
		* @var	viewport
		* @var	fullscreenLayer
		* @var	value		unioned with the above four vars, used for sorting
		*/
		struct RenderQueueKey {
			union {
				struct {
					uint16_t material;
					
					uint16_t depth;

					uint16_t instance : 14;
					uint16_t translucencyType : 2;

					uint16_t viewportLayer : 3;
					uint16_t viewport : 3;
					uint16_t fullscreenLayer : 2;
				};
				uint64_t value;
			};
		};


		struct RenderEntry {
			RenderQueueKey	key;
			glm::mat4		modelToWorldTransform;

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