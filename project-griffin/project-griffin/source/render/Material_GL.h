#pragma once
#ifndef GRIFFIN_MATERIAL_GL_H_
#define GRIFFIN_MATERIAL_GL_H_

#include <glm/vec3.hpp>
#include <utility/container/handle_map.h>

namespace griffin {
	namespace render {

		#define MAX_MATERIAL_TEXTURES			12
		#define MAX_MATERIAL_TEXTURE_NAME_SIZE	64


		enum MaterialTextureType : uint16_t {
			MaterialTexture_None			= 0,
			MaterialTexture_Diffuse			= 1,			// RGB - diffuse surface color,			A - unused
			MaterialTexture_Diffuse_Opacity = 2,			// RGB - diffuse surface color,			A - opacity
			MaterialTexture_Diffuse_AO		= 3,			// RGB - diffuse surface color,			A - ambient occlusion
			MaterialTexture_Specular		= 4,			// RGB - specular reflectivity color,	A - specular power/shininess
			MaterialTexture_Emissive		= 5,			// RGB - emissive light color,			A - brightness
			MaterialTexture_Normal			= 6,			// RGB - normal map,					A - unused
			MaterialTexture_Normal_Height	= 7,			// RGB - normal map,					A - height map
			MaterialTexture_Metallic_Reflectivity_AO = 8	// R - metallic, G - reflectivity, B - ambient occlusion, A - *Unused/Available*
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
			char						name[MAX_MATERIAL_TEXTURE_NAME_SIZE] = {};
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
			uint64_t	shaderKey = 0;		//<! shader key combines vertex and material flags, shader manager stores ubershader by key
			uint8_t		numTextures = 0;
			uint8_t		_padding_0[7] = {};
			// 88
			MaterialTexture textures[MAX_MATERIAL_TEXTURES]; // 80 * 12 = 960
			// 1048
		};

	}
}

#endif