#include "../ShaderProgram_GL.h"
#include <gl/glew.h>
#include <SDL.h>
#include <vector>

namespace griffin {
	namespace render {

		using std::vector;


		// class Shader_GL

		Shader_GL::~Shader_GL()
		{
			glDeleteShader(m_shaderId);
		}

		bool Shader_GL::compileShader(unsigned int shaderType)
		{
			GLint result = GL_FALSE;
			int infoLogLength = 0;

			// Compile Shader
			SDL_Log("Compiling shader");
			GLuint shaderId = glCreateShader(shaderType);
			char const* sourcePointer = m_shaderCode.c_str();
			glShaderSource(shaderId, 1, &sourcePointer, nullptr);
			glCompileShader(shaderId);

			// Check Shader
			glGetShaderiv(shaderId, GL_COMPILE_STATUS, &result);
			glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
			if (infoLogLength > 0) {
				vector<char> shaderErrorMessage(infoLogLength);
				glGetShaderInfoLog(shaderId, infoLogLength, nullptr, &shaderErrorMessage[0]);
				SDL_Log(&shaderErrorMessage[0]);
			}

			m_shaderId = shaderId;
			
			return (result == GL_TRUE);
		}


		// class ShaderProgram_GL

		ShaderProgram_GL::~ShaderProgram_GL()
		{
			glDeleteProgram(m_programId);
		}

		bool ShaderProgram_GL::linkProgram() {
			GLint result = GL_FALSE;
			int infoLogLength = 0;

			// Link the program
			SDL_Log("Linking program");
			GLuint programId = glCreateProgram();
			glAttachShader(programId, m_vertexShaderId);
			glAttachShader(programId, m_fragmentShaderId);
			glLinkProgram(programId);

			// Check the program
			glGetProgramiv(programId, GL_LINK_STATUS, &result);
			glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &infoLogLength);
			if (infoLogLength > 0) {
				vector<char> programErrorMessage(infoLogLength);
				glGetProgramInfoLog(programId, infoLogLength, nullptr, &programErrorMessage[0]);
				SDL_Log(&programErrorMessage[0]);
			}

			m_programId = programId;

			return (result == GL_TRUE);
		}

		void ShaderProgram_GL::useProgram() const
		{
			glUseProgram(m_programId);
		}

	}
}