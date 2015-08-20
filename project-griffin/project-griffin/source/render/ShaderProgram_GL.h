#pragma once
#ifndef GRIFFIN_SHADER_PROGRAM_GL_H_
#define GRIFFIN_SHADER_PROGRAM_GL_H_

#include <string>
#include <memory>
#include <vector>
// TEMP
#include <SDL_log.h>

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
				SDL_Log("moving shader with m_shaderId = %d", m_shaderId);
				other.m_shaderId = 0;
				other.m_shaderType = 0;
			}

			Shader_GL(const Shader_GL& other) = delete;

			~Shader_GL();

			bool			compileShader(const string& shaderCode, unsigned int shaderType);

			unsigned int	getShaderId() const { return m_shaderId; }

		private:
			unsigned int	m_shaderId = 0;
			unsigned int	m_shaderType = 0;
		};
		

		class ShaderProgram_GL {
		public:
			explicit ShaderProgram_GL() = default;

			explicit ShaderProgram_GL(string shaderCode) :
				m_shaderCode(std::move(shaderCode))
			{}

			ShaderProgram_GL(ShaderProgram_GL&& other) :
				m_programId{ other.m_programId },
				m_numShaders{ other.m_numShaders },
				m_shaderCode(std::move(other.m_shaderCode))
			{
				SDL_Log("moving shader program with m_programId = %d", m_programId);
				other.m_programId = 0;
				for (uint32_t s = 0; s < other.m_numShaders; ++s) {
					m_shaders[s] = std::move(other.m_shaders[s]);
				}
				other.m_numShaders = 0;
			}

			ShaderProgram_GL(const Shader_GL&) = delete;
			
			~ShaderProgram_GL();

			bool				compileAndLinkProgram();

			unsigned int		getProgramId() const { return m_programId; }

			void				useProgram() const;

		private:
			unsigned int		m_programId = 0;
			uint32_t			m_numShaders = 0;
			Shader_GL			m_shaders[5] = {};
			string				m_shaderCode;
		};


		typedef shared_ptr<ShaderProgram_GL> ShaderProgramPtr;
	}
}

#endif