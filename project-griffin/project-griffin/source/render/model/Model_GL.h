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

	namespace render {

		// the model is an in-engine wrapper for a mesh
		class Model_GL {
		public:
			typedef std::vector<ResourceHandle<Texture2D_GL>>		TextureList;
			// OR vector<Id_T> ???
			// OR vector<shared_ptr<Texture2D_GL>> ???
			typedef std::vector<ResourceHandle<ShaderProgram_GL>>	ShaderList;
			// OR vector<uint16_t> ???
			// and how do we relate the textures/shaders stored here back to the relevant material texture / material, or do we even need to??

			Mesh_GL		m_mesh;
			TextureList	m_textures;
			ShaderList	m_shaders;

		};

	}
}

#endif