#pragma once
#ifndef GRIFFIN_SHADER_PROGRAM_GL_
#define GRIFFIN_SHADER_PROGRAM_GL_

#include <string>
#include <memory>

namespace griffin {
	namespace render {
		
		using std::string;


		class Shader_GL {
		public:
			explicit Shader_GL() = default;
			explicit Shader_GL(string shaderCode) :
				m_shaderCode(std::move(shaderCode))
			{}
			//Shader_GL(Shader_GL&& other);
			//Shader_GL(const Shader_GL&) = delete;
			~Shader_GL();

			bool compileShader(unsigned int shaderType);

			unsigned int getShaderId() const { return m_shaderId; }

		private:
			unsigned int m_shaderId = 0;
			string m_shaderCode;
		};
		

		class ShaderProgram_GL {
		public:
			explicit ShaderProgram_GL() = default;
			explicit ShaderProgram_GL(const Shader_GL& vertexShader, const Shader_GL& fragmentShader) :
				m_vertexShaderId(vertexShader.getShaderId()),
				m_fragmentShaderId(fragmentShader.getShaderId())
			{}
			//ShaderProgram_GL(ShaderProgram_GL&& other);
			//ShaderProgram_GL(const ShaderProgram_GL&) = delete;
			~ShaderProgram_GL();

			bool linkProgram();

			unsigned int getProgramId() const { return m_programId; }

			void useProgram() const;

		private:
			unsigned int m_programId = 0;
			unsigned int m_vertexShaderId = 0;
			unsigned int m_fragmentShaderId = 0;
		};

	}
}

#endif