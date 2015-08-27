#pragma once
#ifndef GRIFFIN_SHADERMANAGER_GL_H_
#define GRIFFIN_SHADERMANAGER_GL_H_

#include <cstdint>
#include <cassert>
#include <vector>
#include <string>
#include <utility/memory_reserve.h>
#include <render/ShaderProgram_GL.h>

namespace griffin {
	namespace render {

		/**
		* @struct ShaderKey
		* All of the parameters in this key tie in to the ubershader with a compile time ifdef
		*	to generate a unique permutation of the shader for each unique key. The shader manager
		*	stores and identifies needed shaders by this key. Non-ubershader programs just use key
		*	value of 0.
		* @var	numDiffuseTextures	Up to 4 diffuse textures can be blended. for the first diffuse
		*			texture, the channels are interpreted by the present of either the
		*			hasFirstDiffuseTexture or hasFirstDiffuseOpacityTexture flag. For the next 3
		*			diffuse textures, alpha is always used for blending with the base color
		*/
		struct ShaderKey {
			union {
				struct {
					uint8_t isUbershader				: 1;	//<! 1 = shader is an ubershader permutation, 0 = unique shader
					uint8_t hasFirstDiffuseMap			: 1;	//<! mutually exclusive with hasFirstDiffuseOpacityTexture
					uint8_t hasFirstDiffuseOpacityMap	: 1;	//<! mutually exclusive with hasFirstDiffuseTexture
					uint8_t hasSpecularMap				: 1;
					uint8_t hasEmissiveMap				: 1;
					uint8_t hasNormalMap				: 1;
					uint8_t hasHeightMap				: 1;
					uint8_t hasMetallicMap				: 1;	//<! uses R channel in MaterialTexture_Metallic_Reflectivity_AO texture
					// 1
					uint8_t hasReflectivityMap			: 1;	//<! uses G channel in MaterialTexture_Metallic_Reflectivity_AO texture
					uint8_t hasAOMap					: 1;	//<! uses B channel in MaterialTexture_Metallic_Reflectivity_AO texture
					uint8_t numDiffuseTextures			: 2;
					uint8_t usesVertexColorForDiffuse	: 1;	//<! whether or not to multiply vertex color channel 1 into base diffuse, TODO: support or no? Use uniform for color channel index
					uint8_t isReflective				: 1;	//<! reflects environment map
					uint8_t isTranslucent				: 1;	//<! requires alpha blending and depth sorting, rendered in seperate pass after deferred pass
					uint8_t usesAlphaMask				: 1;	//<! when hasFirstDiffuseOpacityMap, alpha channel of diffuse texture 0 is treated as on/off alpha mask
					// 2
					uint8_t usesBumpMapping				: 1;	//<! uses normalmap to do bumpmapping
					uint8_t usesDisplacementMapping		: 1;	//<! uses normalmap and heightmap to do displacement mapping

					uint8_t _padding_0					: 6;
					// 3

					uint8_t _padding_end[5];
				};

				uint64_t	value;
			};
		};


		/**
		*
		*/
		class ShaderManager_GL {
		public:
			explicit ShaderManager_GL()
			{
				m_shaderPrograms.reserve(RESERVE_SHADER_PROGRAMS);
			}

			~ShaderManager_GL();

			ShaderProgram_GL& getShaderProgram(uint16_t index)
			{
				assert(index >= 0 && index < m_shaderPrograms.size() && "index out of range");
				return m_shaderPrograms[index];
			};

			/**
			*
			*/
			uint16_t addShaderProgram(ShaderProgram_GL&& program);

			/**
			* This function will either return the index of an already-compiled shader program or
			* cause a new shader permutation to be compiled based on the key input.
			* @param	key		key for shader permutation
			* @returns	index of compiled and ready shader program
			* @throws	if shader program doesn't compile or link
			*/
			uint16_t ensureUbershaderForKey(ShaderKey key);

			/**
			* @param	outIndex	if true is returned, index of the shader is written into this
			* @returns	true if there is already a compiled ubershader for the given key.
			*/
			bool hasUbershaderForKey(ShaderKey key, uint16_t* outIndex = nullptr) const;

			/**
			* Causes all currently built ubershaders to be recompiled and linked. This is useful
			* during development for hot reloading after shader code changes have been made.
			*/
			bool rebuildAllCurrentUbershaders();

			/**
			* Loads the ubershader code used to compile all material permutations. There is a very
			* close tie between the shader code, and the code within this class that sets the
			* compile time conditional flags based on material key parameters.
			*/
			void loadUbershaderCode(const char* filename);

		private:

			// Variables

			struct ShaderIndex {
				ShaderKey	key;
				uint16_t	shaderIndex;
				
				uint8_t		_padding_end[6];
			};

			std::vector<ShaderIndex>		m_index;
			std::vector<ShaderProgram_GL>	m_shaderPrograms;
			std::string						m_ubershaderCode;
		};

	}
}

#endif