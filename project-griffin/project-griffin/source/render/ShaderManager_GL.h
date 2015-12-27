#pragma once
#ifndef GRIFFIN_SHADERMANAGER_GL_H_
#define GRIFFIN_SHADERMANAGER_GL_H_

#include <cstdint>
#include <cassert>
#include <vector>
#include <string>
#include <utility/memory_reserve.h>
#include <render/ShaderProgram_GL.h>
#include <render/Material_GL.h>

namespace griffin {
	namespace render {

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