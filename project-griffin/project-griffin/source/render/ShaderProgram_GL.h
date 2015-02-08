#pragma once
#ifndef GRIFFIN_SHADER_PROGRAM_GL_
#define GRIFFIN_SHADER_PROGRAM_GL_

#include <string>
#include <memory>
// TEMP
#include <SDL_log.h>

namespace griffin {
	namespace render {
		
		using std::string;


		class Shader_GL {
		public:
			explicit Shader_GL() = default;
			
			explicit Shader_GL(string shaderCode) :
				m_shaderCode(std::move(shaderCode))
			{
				SDL_Log("creating shader by string");
			}

			Shader_GL(Shader_GL&& other)
			{
				SDL_Log("moving shader with m_shaderId = %d", m_shaderId);
				m_shaderId = other.m_shaderId;
				other.m_shaderId = 0;
				m_shaderCode = std::move(other.m_shaderCode);
			}

			Shader_GL(const Shader_GL& other) = delete;

			~Shader_GL();

			bool			compileShader(unsigned int shaderType);

			unsigned int	getShaderId() const { return m_shaderId; }

		private:
			unsigned int	m_shaderId = 0;
			string			m_shaderCode;
		};
		

		class ShaderProgram_GL {
		public:
			explicit ShaderProgram_GL() = default;

			explicit ShaderProgram_GL(const Shader_GL& vertexShader, const Shader_GL& fragmentShader) :
				m_vertexShaderId(vertexShader.getShaderId()),
				m_fragmentShaderId(fragmentShader.getShaderId())
			{
				SDL_Log("creating program");
			}
			
			//ShaderProgram_GL(ShaderProgram_GL&& other);
			
			ShaderProgram_GL(const Shader_GL&) = delete;
			
			~ShaderProgram_GL();

			bool			linkProgram();

			unsigned int	getProgramId() const { return m_programId; }

			void			useProgram() const;

		private:
			unsigned int	m_programId = 0;
			unsigned int	m_vertexShaderId = 0;
			unsigned int	m_fragmentShaderId = 0;
		};

	}
}

#endif