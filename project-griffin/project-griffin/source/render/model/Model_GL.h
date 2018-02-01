#pragma once
#ifndef GRIFFIN_MODEL_GL_H_
#define GRIFFIN_MODEL_GL_H_

#include <vector>
#include <string>
#include "Mesh_GL.h"
#include <resource/Resource.h>
#include <render/texture/Texture2D_GL.h>
#include <render/ShaderProgram_GL.h>
#include <render/Render.h>
#include <entity/EntityTypedefs.h>


namespace griffin {
	using namespace resource;
	using std::vector;

	struct Engine;

	namespace scene {
		class Scene;
	}

	namespace render {

		/**
		* Model_GL is the engine's wrapper for a mesh. The mesh contains all relevant data to be
		* loaded from disk, and is constructed in a way that can be loaded and persisted quickly.
		* This wrapper holds the mesh and shared_ptr's to required child resources like textures
		* materials, and shader programs. Model_GL is the resource constructed by the resource
		* loading system. The resource size refers to the "on disk" memory which is just the mesh
		* and doesn't include space for the shared_ptr's and other fields.
		*/
		class Model_GL {
		public:
			typedef vector<ResourceHandle<Texture2D_GL>>		TextureList;
			// OR vector<Id_T> ???
			// OR vector<shared_ptr<Texture2D_GL>> ???
			typedef vector<ResourceHandle<ShaderProgram_GL>>	ShaderList;
			// OR vector<uint16_t> ???
			// and how do we relate the textures/shaders stored here back to the relevant material texture / material, or do we even need to??

			struct RenderEntryList {
				vector<RenderQueue::KeyType>	keys;
				vector<RenderEntry>				entries;
			};

			// Constructors
			explicit Model_GL() {}

			/**
			* Constructor used by resource builder callback
			*/
			explicit Model_GL(Mesh_GL&& mesh) :
				m_mesh(std::forward<Mesh_GL>(mesh))
			{}

			Model_GL(Model_GL&& other) :
				m_mesh(std::move(other.m_mesh)),
				m_renderEntries(std::move(other.m_renderEntries)),
				m_textures(std::move(other.m_textures)),
				m_shaderPrograms(std::move(other.m_shaderPrograms)),
				m_resourcesLoaded{ other.m_resourcesLoaded }
			{
				other.m_resourcesLoaded = false;
			}

			Model_GL(const Model_GL&) = delete;

			void render(entity::ComponentId modelInstanceId, scene::Scene& scene, uint8_t viewport, Engine& engine);
			void draw(entity::ComponentId modelInstanceId, int drawSetIndex);
			
			/**
			* Builds a list of render keys/entries  for
			* the owner model.
			*/
			void initRenderEntries(Model_GL& owner);

			/**
			* Builds list of render keys/entries for optimized submission to the render queue. Can
			* be called on loading thread since no OpenGL calls are made.
			*/
			void initRenderEntries();

			/**
			* Causes all mesh material textures and shaders to be queued up for asynchronous
			* loading, and stores all resource handles containing the futures. Can be called from
			* the loading thread since no OpenGL calls are directly made, this only creates the
			* resource loading tasks. Once all resources are loaded, the m_resourcesLoaded bool is
			* set to true, and the model is ready to be rendered.
			*/
			void loadMaterialResources(const std::wstring& filePath);

			/**
			* Creates OpenGL buffers in the internal mesh. Call this from the OpenGL thread after
			* the mesh is constructed.
			*/
			void createBuffers()
			{
				m_mesh.createBuffersFromInternalMemory();
			}

			// Variables
			Mesh_GL			m_mesh; // should models have a collection of meshes here?
			RenderEntryList	m_renderEntries;
			TextureList		m_textures;
			ShaderList		m_shaderPrograms;
			bool			m_resourcesLoaded = false;	//<! true when all resource asyncronous loading completed, model can be rendered

		};

	}
}

#endif