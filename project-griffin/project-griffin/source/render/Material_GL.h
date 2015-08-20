#pragma once
#ifndef GRIFFIN_MATERIAL_GL_H_
#define GRIFFIN_MATERIAL_GL_H_

#include <glm/vec3.hpp>
#include <utility/container/handle_map.h>

namespace griffin {
	namespace render {

		#define GRIFFIN_MAX_MATERIAL_TEXTURES			12
		#define GRIFFIN_MAX_MATERIAL_TEXTURE_NAME_SIZE	64

		// TODO: which material property is most commonly needed along with diffuse, to go in alpha channel when
		// both transparency and specular reflectivity map aren't needed? I'm thinking metallic or shininess
		enum MaterialTextureType : uint16_t {
			MaterialTexture_None			= 0,
			MaterialTexture_Diffuse			= 1,			// RGB - diffuse surface color,			A - *Unused/Available*
			MaterialTexture_Diffuse_Opacity	= 2,			// RGB - diffuse surface color,			A - opacity
			MaterialTexture_Specular		= 3,			// RGB - specular reflectivity color,	A - specular power/shininess
			MaterialTexture_Emissive		= 4,			// RGB - emissive light color,			A - brightness
			MaterialTexture_Normal_Height	= 5,			// RGB - normal map,					A - height map
			MaterialTexture_Metallic_Reflectivity_AO = 6	// R - Metallic, G - Reflectivity, B - Ambient Occlusion, A - *Unused/Available*
		};

		enum MaterialTextureMappingMode : uint8_t {
			MaterialTextureMappingMode_None   = 0,
			MaterialTextureMappingMode_Wrap   = 1,
			MaterialTextureMappingMode_Clamp  = 2,
			MaterialTextureMappingMode_Decal  = 3,
			MaterialTextureMappingMode_Mirror = 4
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


		struct MaterialTexture {
			Id_T						textureResourceHandle = NullId_T;
			MaterialTextureType			textureType = MaterialTexture_None;
			uint8_t						uvChannelIndex = 0;
			MaterialTextureMappingMode	textureMappingModeU = MaterialTextureMappingMode_None;
			MaterialTextureMappingMode	textureMappingModeV = MaterialTextureMappingMode_None;
			uint8_t						_padding_0[3] = {};
			char						name[GRIFFIN_MAX_MATERIAL_TEXTURE_NAME_SIZE] = {};
		};


		class Material_GL {
		public:
			glm::vec3	diffuseColor = {};
			glm::vec3	ambientColor = {};
			glm::vec3	specularColor = {};
			glm::vec3	emissiveColor = {};
			// 48
			float		opacity = 1.0f;		
			float		reflectivity = 0;
			float		shininess = 0;
			float		metallic = 0;
			// 64
			Id_T		shaderResourceHandle = NullId_T;
			uint32_t	shaderKey = 0;		//<! shader key combines vertex and material flags, shader manager stores ubershader by key
			uint32_t	numTextures = 0;	// don't need the full 32 bits only one, the rest is padding, get bytes from here if needed
			// 80
			MaterialTexture textures[GRIFFIN_MAX_MATERIAL_TEXTURES];
			// 272

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