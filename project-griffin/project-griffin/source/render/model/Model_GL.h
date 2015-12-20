#pragma once
#ifndef GRIFFIN_MODEL_GL_H_
#define GRIFFIN_MODEL_GL_H_

#include <vector>
#include "Mesh_GL.h"
#include <resource/Resource.h>
#include <render/texture/Texture2D_GL.h>
#include <render/ShaderProgram_GL.h>

namespace griffin {
	using namespace resource;
	
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
			typedef std::vector<ResourceHandle<Texture2D_GL>>		TextureList;
			// OR vector<Id_T> ???
			// OR vector<shared_ptr<Texture2D_GL>> ???
			typedef std::vector<ResourceHandle<ShaderProgram_GL>>	ShaderList;
			// OR vector<uint16_t> ???
			// and how do we relate the textures/shaders stored here back to the relevant material texture / material, or do we even need to??

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
				m_textures(std::move(other.m_textures)),
				m_shaderPrograms(std::move(other.m_shaderPrograms)),
				m_resourcesLoaded{ other.m_resourcesLoaded }
			{
				other.m_resourcesLoaded = false;
			}

			Model_GL(const Model_GL&) = delete;

			void render(Id_T entityId, scene::Scene& scene, uint8_t viewport, Engine& engine);
			void draw(Id_T entityId, int drawSetIndex);
			
			// Variables
			Mesh_GL		m_mesh; // should models have a collection of meshes here?
			TextureList	m_textures;
			ShaderList	m_shaderPrograms;
			bool		m_resourcesLoaded = false;	//<! true when all resource asyncronous loading completed, model can be rendered

		};

	}
}

#endif