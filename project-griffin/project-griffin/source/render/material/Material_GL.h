#pragma once
#ifndef GRIFFIN_MATERIAL_GL_H_
#define GRIFFIN_MATERIAL_GL_H_

#include <glm/vec3.hpp>
#include <resource/Resource.h>
#include <render/texture/Texture2D_GL.h>

namespace griffin {
	namespace render {
		using resource::ResourceHandle;

		#define GRIFFIN_MAX_MATERIAL_TEXTURES	8

		enum MaterialTextureType : uint16_t {
			MaterialTexture_Diffuse      = 0,
			MaterialTexture_Specular     = 1,
			MaterialTexture_Ambient      = 2,
			MaterialTexture_Emissive     = 3,
			MaterialTexture_Normals      = 4,
			MaterialTexture_Height       = 5,
			MaterialTexture_Shininess    = 6,
			MaterialTexture_Opacity      = 7
		};

		struct MaterialTexture {
			Id_T				textureResourceHandle = NullId_T;
			MaterialTextureType	textureType = MaterialTexture_Diffuse;
			uint8_t				uvChannelIndex = 0;
			
			uint8_t				_padding_end = 0;
		};

		enum MaterialFlags : uint16_t {
			Material_None                = 0,
			Material_Textured            = 1,
			Material_Lit                 = 1 << 1,
			Material_Shadowed            = 1 << 2,
			Material_Reflective          = 1 << 3,
			Material_Transparent         = 1 << 4,
			Material_NormalMapping       = 1 << 5,
			Material_DisplacementMapping = 1 << 6,
			Material_VertexColors        = 1 << 7
		};

		class Material_GL {
		public:
			glm::vec3 diffuseColor = {};
			glm::vec3 ambientColor = {};
			glm::vec3 specularColor = {};
			glm::vec3 emissiveColor = {};
			
			float     opacity = 1.0f;
			float     reflectivity = 0;
			float     shininess = 0;
			float     metallic = 0;

			Id_T      shaderResourceHandle = NullId_T;

			//std::vector<ResourceHandle<Texture2D_GL>> textures; // maybe need sparse array for simpler binding to shaders?
			MaterialTexture textures[GRIFFIN_MAX_MATERIAL_TEXTURES];

			uint32_t  shaderKey = 0;	//<! shader key combines vertex and material flags, shader manager stores ubershader by key

			// Functions

			// TODO: reenable this check in terms of presence of the textures and shader program id
			bool allResourcesAvailable() const {
				//for (const auto& t : textures) {
				//	if (!t.isAvailable()) { return false; }
				//}
				return true;
			}
		};

	}
}

#endif