#pragma once
#ifndef GRIFFIN_SHADER_PROGRAM_GL_H_
#define GRIFFIN_SHADER_PROGRAM_GL_H_

#include <string>
#include <memory>
#include <vector>
#include <utility/memory_reserve.h>


namespace griffin {
	namespace render {
		
		using std::string;
		using std::vector;
		using std::shared_ptr;


		class Shader_GL {
		public:
			explicit Shader_GL() = default;

			Shader_GL(Shader_GL&& other) :
				m_shaderId{ other.m_shaderId },
				m_shaderType{ other.m_shaderType }
			{
				other.m_shaderId = 0;
				other.m_shaderType = 0;
			}

			Shader_GL(const Shader_GL& other) = delete;

			~Shader_GL();

			bool compileShader(const char* shaderSource, unsigned int shaderType);

			unsigned int getShaderId() const { return m_shaderId; }

		private:
			unsigned int	m_shaderId = 0;
			unsigned int	m_shaderType = 0;
		};
		

		class ShaderProgram_GL {
		public:
			explicit ShaderProgram_GL() = default;

			explicit ShaderProgram_GL(string shaderCode) :
				m_shaderCode(std::move(shaderCode))
			{
				m_preprocessorMacros.reserve(RESERVE_SHADER_PROGRAM_PREPROCESSORS);
			}

			ShaderProgram_GL(ShaderProgram_GL&& other);

			ShaderProgram_GL(const Shader_GL&) = delete;
			
			~ShaderProgram_GL();

			/**
			* @param	shaderCode	null-terminated string containing shader code, if nullptr is
			*	passed, code is taken from m_shaderCode which must have been set at construction.
			* @returns	true if compilation and link succeed, false on failure
			*/
			bool compileAndLinkProgram(const char* shaderCode = nullptr);

			void addPreprocessorMacro(const char* preprocessor)
			{
				m_preprocessorMacros += string(preprocessor) + "\n";
			}

			unsigned int getProgramId() const { return m_programId; }

			void useProgram() const;

		private:
			unsigned int		m_programId = 0;
			uint32_t			m_numShaders = 0;
			Shader_GL			m_shaders[5] = {};
			string				m_shaderCode;
			string				m_preprocessorMacros = "";
		};


		typedef shared_ptr<ShaderProgram_GL> ShaderProgramPtr;
	}
}

#endif