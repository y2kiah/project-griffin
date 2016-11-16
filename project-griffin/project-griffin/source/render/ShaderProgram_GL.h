#pragma once
#ifndef GRIFFIN_SHADER_PROGRAM_GL_H_
#define GRIFFIN_SHADER_PROGRAM_GL_H_

#include <string>
#include <memory>
#include <vector>
#include <utility/memory_reserve.h>
#include "ShaderProgramLayouts_GL.h"

namespace griffin {
	namespace render {
		
		using std::string;
		using std::wstring;
		using std::vector;
		using std::shared_ptr;

		/**
		*
		*/
		class Shader_GL {
		public:
			explicit Shader_GL() = default;

			Shader_GL(Shader_GL&& other) = default;/* :
				m_shaderId{ other.m_shaderId },
				m_shaderType{ other.m_shaderType }
			{
				other.m_shaderId = 0;
				other.m_shaderType = 0;
			}*/

			Shader_GL(const Shader_GL& other) = delete;

			Shader_GL& operator=(Shader_GL&&) = default;

			~Shader_GL();

			bool compileShader(const char* shaderSource, unsigned int shaderType);

			unsigned int getShaderId() const { return m_shaderId; }
			unsigned int getShaderType() const { return m_shaderType; }

		private:
			unsigned int	m_shaderId = 0;
			unsigned int	m_shaderType = 0;
		};
		

		class ShaderProgram_GL {
		public:
			explicit ShaderProgram_GL() = default;

			explicit ShaderProgram_GL(string shaderCode, wstring programPath = L"") :
				m_shaderCode(std::move(shaderCode)),
				m_programPath(std::move(programPath))
			{
				m_preprocessorMacros.reserve(RESERVE_SHADER_PROGRAM_PREPROCESSORS);
			}

			ShaderProgram_GL(ShaderProgram_GL&& other);

			ShaderProgram_GL(const ShaderProgram_GL&) = delete;

			~ShaderProgram_GL();

			/**
			* @param	shaderCode	null-terminated string containing shader code, if nullptr is
			*	passed, code is taken from m_shaderCode which must have been set at construction.
			* @returns	true if compilation and link succeed, false on failure
			*/
			bool compileAndLinkProgram(const char* shaderCode = nullptr);

			void bindUniformBuffer(UBOType uboType, unsigned int uboHandle);

			bool loadProgramBinaryFromMemory(unsigned char* data, size_t size);
			bool loadProgramBinaryFromFile(const char* filename);
			bool writeProgramBinaryFile(const char* filename) const;

			void deserialize(std::istream& in);
			void serialize(std::ostream& out);

			void addPreprocessorMacro(const char* preprocessor)
			{
				m_preprocessorMacros += string(preprocessor) + "\n";
			}

			unsigned int getProgramId() const { return m_programId; }

			void useProgram() const;

		private:
			unsigned int		m_programId = 0;
			uint32_t			m_numShaders = 0;
			unsigned int		m_blockIndex[UBOTypeCount] = {};	// block index of each UBO type
			Shader_GL			m_shaders[5] = {};
			string				m_shaderCode;
			string				m_preprocessorMacros = "";
			wstring				m_programPath;
		};


		typedef shared_ptr<ShaderProgram_GL> ShaderProgramPtr;
	}
}

#endif