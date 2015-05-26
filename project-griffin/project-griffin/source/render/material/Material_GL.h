#pragma once
#ifndef GRIFFIN_MATERIAL_GL_H_
#define GRIFFIN_MATERIAL_GL_H_

#include <glm/vec3.hpp>
#include <resource/Resource.h>
#include <render/texture/Texture2D_GL.h>

namespace griffin {
	namespace render {
		using resource::ResourceHandle;

		/*
		enum MaterialFlags : uint16_t {
			Material_None        = 0,
			Material_Ambient     = 1,
			Material_Diffuse     = 1 << 1,
			Material_Specular    = 1 << 2,
			Material_Emissive    = 1 << 3,
			Material_Reflective  = 1 << 4,
			Material_Transparent = 1 << 5
		};

		struct Material_GL {
			VertexFlags   vertexRequirementFlags;
			MaterialFlags materialFlags;

		};
		*/

		enum MaterialTextureChannel {
			MaterialTexture_Diffuse = 0,
			MaterialTexture_Specular = 1,
			MaterialTexture_Ambient = 2,
			MaterialTexture_Emissive = 3,
			MaterialTexture_Normals = 4,
			MaterialTexture_Height = 5,
			MaterialTexture_Shininess = 6,
			MaterialTexture_Opacity = 7,
			MaterialTexture_Displacement = 8,
			MaterialTexture_Reflection = 9
		};


		class Material_GL {
		public:
			glm::vec3 diffuseColor = {};
			glm::vec3 ambientColor = {};
			glm::vec3 specularColor = {};
			glm::vec3 emissiveColor = {};
			glm::vec3 reflectiveColor = {};
			glm::vec3 transparentColor = {};
			
			float     opacity = 1.0f;
			float     reflectivity = 0;
			float     refracti = 0;
			float     shininess = 0;
			float     metallic = 0;

			std::vector<ResourceHandle<Texture2D_GL>> textures; // maybe need sparse array for simpler binding to shaders?

			// Functions

			bool allResourcesAvailable() const {
				for (const auto& t : textures) {
					if (!t.isAvailable()) { return false; }
				}
				return true;
			}
		};

	}
}

#endif