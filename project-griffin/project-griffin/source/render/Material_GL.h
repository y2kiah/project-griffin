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
			MaterialTexture_Diffuse_Opacity = 2,			// RGB - diffuse surface color,			A - opacity (alpha blend)
			MaterialTexture_Diffuse_OpacityMask = 3,		// RGB - diffuse surface color,			A - opacity (no blend, alpha test only)
			MaterialTexture_Diffuse_Occlusion = 4,			// RGB - diffuse surface color,			A - occlusion
			MaterialTexture_Diffuse_Height	= 5,			// RGB - diffuse surface color,			A - height
			MaterialTexture_Diffuse_Specular = 6,			// RGB - diffuse surface color,			A - specular
			MaterialTexture_Emissive		= 7,			// RGB - emissive light color,			A - brightness
			MaterialTexture_Normal			= 8,			// RGB - normal map,					A - unused
			MaterialTexture_Normal_Height	= 9,			// RGB - normal map,					A - height
			MaterialTexture_Normal_Specular = 10,			// RGB - normal map,					A - specular
			MaterialTexture_Specular_Metallic_Reflectivity_Occlusion = 11	// R - specular, G - metallic, B - reflectivity, A - occlusion
		};

		enum MaterialTextureMappingMode : uint8_t {
			MaterialTextureMappingMode_None   = 0,
			MaterialTextureMappingMode_Wrap   = 1,
			MaterialTextureMappingMode_Clamp  = 2,
			MaterialTextureMappingMode_Decal  = 3,
			MaterialTextureMappingMode_Mirror = 4
		};

		/**
		* @struct ShaderKey
		* All of the parameters in this key tie in to the ubershader with a compile time ifdef
		*	to generate a unique permutation of the shader for each unique key. The shader manager
		*	stores and identifies needed shaders by this key. Non-ubershader programs just use key
		*	value of 0.
		* @var	numDiffuseTextures	Up to 4 diffuse textures can be blended. for the first diffuse
		*			texture, the channels are interpreted by the presence of either the
		*			hasFirstDiffuseMap or hasFirstDiffuseOpacityMap flag. For the next 3
		*			diffuse textures, alpha is always used for blending with the base color
		*/
		struct ShaderKey {
			union {
				struct {
					uint8_t isUbershader				: 1;	//<! 1 = shader is an ubershader permutation, 0 = unique shader
					uint8_t hasFirstDiffuseMap			: 1;	//<! mutually exclusive with hasFirstDiffuseOpacityMap and hasFirstDiffuseAOMap
					uint8_t hasFirstDiffuseOpacityMap	: 1;	//<! mutually exclusive with hasFirstDiffuseMap and hasFirstDiffuseAOMap
					uint8_t hasFirstDiffuseAOMap		: 1;	//<! mutually exclusive with hasFirstDiffuseMap and hasFirstDiffuseOpacityMap
					uint8_t hasSpecularMap				: 1;
					uint8_t hasEmissiveMap				: 1;
					uint8_t hasNormalMap				: 1;
					uint8_t hasNormalHeightMap			: 1;
					// 1
					uint8_t hasMetallicReflectiveAOMap	: 1;
					uint8_t numDiffuseTextures			: 2;
					uint8_t usesVertexColorForDiffuse	: 1;	//<! whether or not to multiply vertex color channel 1 into base diffuse, TODO: support or no? Use uniform for color channel index
					uint8_t isLit						: 1;	//<! accepts scene lighting, if 0 object masked off from lighting using stencil buffer
					uint8_t isReflective				: 1;	//<! reflects environment map
					uint8_t isTranslucent				: 1;	//<! uses alpha blend and depth sort, rendered after deferred pass, either mat. opacity < 1 or usesAlphaBlend=1 cause this to be 1
					uint8_t isShadowed					: 1;	//<! accepts shadows from scene, if 0 object masked off from shadows using stencil buffer???
					// 2
					uint8_t castsShadow					: 1;	//<! casts shadow in scene, if 0 object does not cast a shadow
					uint8_t usesAlphaBlend				: 1;	//<! when hasFirstDiffuseOpacityMap=1, alpha channel of diffuse texture 0 is used for alpha blend
					uint8_t usesAlphaTest				: 1;	//<! when hasFirstDiffuseOpacityMap=1, alpha channel of diffuse texture 0 is treated as on/off alpha mask
					uint8_t usesBumpMapping				: 1;	//<! uses normalmap to do bumpmapping
					uint8_t usesDisplacementMapping		: 1;	//<! uses normalmap and heightmap to do displacement mapping

					uint8_t _padding_0					: 3;
					// 3

					uint8_t _padding_end[5];
					// 8
					// TODO: how to handle LOD, e.g. turn off bump/displacement mapping with distance from camera
				};

				uint64_t	value;
			};
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


		struct Material_GL {
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
			ShaderKey	shaderKey = {};		//<! shader key combines vertex and material flags, shader manager stores ubershader by key
			uint8_t		numTextures = 0;
			uint8_t		_padding_0[7] = {};
			// 88
			MaterialTexture textures[MAX_MATERIAL_TEXTURES]; // 80 * 12 = 960
			// 1048
		};

	}
}

#endif