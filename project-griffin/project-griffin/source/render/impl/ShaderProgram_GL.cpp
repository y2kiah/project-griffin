#include "../ShaderProgram_GL.h"
#include <gl/glew.h>
#include <vector>

namespace griffin {
	namespace render {

		using std::vector;


		// class Shader_GL

		Shader_GL::~Shader_GL()
		{
			SDL_Log("deleting shader with m_shaderId = %d", m_shaderId);
			if (m_shaderId != 0) {
				SDL_Log("deleting shader in opengl land");
				glDeleteShader(m_shaderId);
			}
		}

		bool Shader_GL::compileShader(const string& shaderCode, unsigned int shaderType)
		{
			GLint result = GL_FALSE;
			int infoLogLength = 0;

			// Compile Shader
			SDL_Log("Compiling shader");
			GLuint shaderId = glCreateShader(shaderType);
			
			const char *shaderDefine = nullptr;
			switch (shaderType) {
				case GL_VERTEX_SHADER:
					shaderDefine = "#version 440 core\n#define _VERTEX_\n";
					break;
				case GL_FRAGMENT_SHADER:
					shaderDefine = "#version 440 core\n#define _FRAGMENT_\n";
					break;
				case GL_GEOMETRY_SHADER:
					shaderDefine = "#version 440 core\n#define _GEOMETRY_\n";
					break;
			}

			const char* source[2] = {
				shaderDefine,
				shaderCode.c_str()
			};
			glShaderSource(shaderId, 2, source, nullptr);
			glCompileShader(shaderId);

			// Check Shader
			glGetShaderiv(shaderId, GL_COMPILE_STATUS, &result);
			glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
			if (infoLogLength > 0) {
				vector<char> shaderErrorMessage(infoLogLength);
				glGetShaderInfoLog(shaderId, infoLogLength, nullptr, &shaderErrorMessage[0]);
				SDL_Log(&shaderErrorMessage[0]);
			}

			if (result == GL_TRUE) {
				m_shaderId = shaderId;
				m_shaderType = shaderType;
				return true;
			}
			
			return false;
		}


		// class ShaderProgram_GL

		ShaderProgram_GL::~ShaderProgram_GL()
		{
			if (m_programId != 0) {
				SDL_Log("deleting program");
				glDeleteProgram(m_programId);
			}
		}

		bool ShaderProgram_GL::compileAndLinkProgram() {
			bool hasGeometryStage = (m_shaderCode.find("_GEOMETRY_", 0, 10) != string::npos);
			
			m_shaders.reserve(hasGeometryStage ? 3 : 2);
			
			// Compile code as vertex shader (defines _VERTEX_)
			m_shaders.emplace_back();
			bool ok = m_shaders.back().compileShader(m_shaderCode, GL_VERTEX_SHADER);
			
			if (hasGeometryStage) {
				// Compile code as geometry shader (defines _GEOMETRY_)
				m_shaders.emplace_back();
				ok = ok && m_shaders.back().compileShader(m_shaderCode, GL_GEOMETRY_SHADER);
			}

			// Compile code as fragment shader (defines _FRAGMENT_)
			m_shaders.emplace_back();
			ok = ok && m_shaders.back().compileShader(m_shaderCode, GL_FRAGMENT_SHADER);

			// Link the program
			if (ok) {
				SDL_Log("Linking program");
				GLuint programId = glCreateProgram();
				for (const auto& s : m_shaders) {
					glAttachShader(programId, s.getShaderId());
				}
				glLinkProgram(programId);

				// Check the program
				GLint result = GL_FALSE;
				int infoLogLength = 0;

				glGetProgramiv(programId, GL_LINK_STATUS, &result);
				glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &infoLogLength);
				if (infoLogLength > 0) {
					vector<char> programErrorMessage(infoLogLength);
					glGetProgramInfoLog(programId, infoLogLength, nullptr, &programErrorMessage[0]);
					SDL_Log(&programErrorMessage[0]);
				}

				/*if (hasGeometryStage) {
					glProgramParameteriEXT(programId, GL_GEOMETRY_INPUT_TYPE_EXT, GL_TRIANGLES);
					glProgramParameteriEXT(programId, GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP);
					glProgramParameteriEXT(programId, GL_GEOMETRY_VERTICES_OUT_EXT, 3);
				}*/

				if (result == GL_TRUE) {
					m_programId = programId;
					return true;
				}
			}
			return false;
		}

		void ShaderProgram_GL::useProgram() const
		{
			glUseProgram(m_programId);
		}

	}
}