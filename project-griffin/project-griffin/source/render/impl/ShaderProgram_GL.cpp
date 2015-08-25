#include "../ShaderProgram_GL.h"
#include <gl/glew.h>
#include <vector>
#include <cassert>

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

		bool Shader_GL::compileShader(const char* shaderCode, unsigned int shaderType)
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
				case GL_TESS_CONTROL_SHADER:
					shaderDefine = "#version 440 core\n#define _TESS_CONTROL_\n";
					break;
				case GL_TESS_EVALUATION_SHADER:
					shaderDefine = "#version 440 core\n#define _TESS_EVAL_\n";
					break;
			}

			// TODO: use shaderKey passed in to #define a bunch more stuff for ubershader

			const char* source[2] = {
				shaderDefine,
				shaderCode
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

		bool ShaderProgram_GL::compileAndLinkProgram(const char* shaderCode) {
			if (shaderCode == nullptr) {
				shaderCode = m_shaderCode.c_str();
				assert(m_shaderCode.length() > 0 && "no shader code set");
			}
			else {
				m_shaderCode = shaderCode;
			}
			
			// convert from string find to strnpos?
			bool hasGeometryStage    = (m_shaderCode.find("_GEOMETRY_", 0) != string::npos);
			bool hasTessControlStage = (m_shaderCode.find("_TESS_CONTROL_", 0) != string::npos);
			bool hasTessEvalStage    = (m_shaderCode.find("_TESS_EVAL_", 0) != string::npos);
			
			m_numShaders = (hasGeometryStage ? 3 : 2);

			// Compile code as vertex shader (defines _VERTEX_)
			int currentShader = 0;
			bool ok = m_shaders[currentShader].compileShader(shaderCode, GL_VERTEX_SHADER);
			
			if (hasTessControlStage) {
				++currentShader;
				// Compile code as tesselation control shader (defines _TESS_CONTROL_)
				ok = ok && m_shaders[currentShader].compileShader(shaderCode, GL_TESS_CONTROL_SHADER);
			}

			if (hasTessEvalStage) {
				++currentShader;
				// Compile code as tesselation evaluation shader (defines _TESS_EVAL_)
				ok = ok && m_shaders[currentShader].compileShader(shaderCode, GL_TESS_EVALUATION_SHADER);
			}

			if (hasGeometryStage) {
				++currentShader;
				// Compile code as geometry shader (defines _GEOMETRY_)
				ok = ok && m_shaders[currentShader].compileShader(shaderCode, GL_GEOMETRY_SHADER);
			}

			// Compile code as fragment shader (defines _FRAGMENT_)
			++currentShader;
			ok = ok && m_shaders[currentShader].compileShader(shaderCode, GL_FRAGMENT_SHADER);

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