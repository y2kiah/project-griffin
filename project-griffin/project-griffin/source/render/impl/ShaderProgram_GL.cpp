#include "../ShaderProgram_GL.h"
#include <GL/glew.h>
#include <vector>
#include <cassert>
#include <memory>
#include <SDL_log.h>
#include <utility/enum.h>

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

		bool Shader_GL::compileShader(const char* shaderSource, unsigned int shaderType)
		{
			GLint result = GL_FALSE;
			int infoLogLength = 0;

			// Compile Shader
			SDL_Log("Compiling shader %u", shaderType);

			GLuint shaderId = glCreateShader(shaderType);
			
			glShaderSource(shaderId, 1, &shaderSource, nullptr);
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

		ShaderProgram_GL::ShaderProgram_GL(ShaderProgram_GL&& other) :
			m_programId{ other.m_programId },
			m_numShaders{ other.m_numShaders },
			m_shaderCode(std::move(other.m_shaderCode)),
			m_programPath(std::move(other.m_programPath))
		{
			SDL_Log("moving shader program with m_programId = %d", m_programId);
			other.m_programId = 0;
			for (uint32_t s = 0; s < other.m_numShaders; ++s) {
				m_shaders[s] = std::move(other.m_shaders[s]);
			}
			other.m_numShaders = 0;

			m_preprocessorMacros.reserve(other.m_preprocessorMacros.size());
			m_preprocessorMacros = std::move(other.m_preprocessorMacros);
		}

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
			
			string programPath(m_programPath.begin(), m_programPath.end());
			SDL_Log("compiling program %s", programPath.c_str());

			bool hasGeometryStage    = (m_shaderCode.find("_GEOMETRY_", 0) != string::npos);
			bool hasTessControlStage = (m_shaderCode.find("_TESS_CONTROL_", 0) != string::npos);
			bool hasTessEvalStage    = (m_shaderCode.find("_TESS_EVAL_", 0) != string::npos);

			// Compile code as vertex shader (defines _VERTEX_)
			int currentShader = 0;
			string shaderSource = "#version 440 core\n#define _VERTEX_\n" + m_preprocessorMacros + m_shaderCode;
			bool ok = m_shaders[currentShader].compileShader(shaderSource.c_str(), GL_VERTEX_SHADER);
			
			if (hasTessControlStage) {
				// Compile code as tesselation control shader (defines _TESS_CONTROL_)
				++currentShader;
				shaderSource = "#version 440 core\n#define _TESS_CONTROL_\n" + m_preprocessorMacros + m_shaderCode;
				ok = ok && m_shaders[currentShader].compileShader(shaderSource.c_str(), GL_TESS_CONTROL_SHADER);
			}

			if (hasTessEvalStage) {
				// Compile code as tesselation evaluation shader (defines _TESS_EVAL_)
				++currentShader;
				shaderSource = "#version 440 core\n#define _TESS_EVAL_\n" + m_preprocessorMacros + m_shaderCode;
				ok = ok && m_shaders[currentShader].compileShader(shaderSource.c_str(), GL_TESS_EVALUATION_SHADER);
			}

			if (hasGeometryStage) {
				// Compile code as geometry shader (defines _GEOMETRY_)
				++currentShader;
				shaderSource = "#version 440 core\n#define _GEOMETRY_\n" + m_preprocessorMacros + m_shaderCode;
				ok = ok && m_shaders[currentShader].compileShader(shaderSource.c_str(), GL_GEOMETRY_SHADER);
			}

			// Compile code as fragment shader (defines _FRAGMENT_)
			++currentShader;
			shaderSource = "#version 440 core\n#define _FRAGMENT_\n" + m_preprocessorMacros + m_shaderCode;
			ok = ok && m_shaders[currentShader].compileShader(shaderSource.c_str(), GL_FRAGMENT_SHADER);


			m_numShaders = currentShader + 1;

			// Link the program
			if (ok) {
				SDL_Log("Linking program %s", programPath.c_str());
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

		/**
		* Shader_GL binary file header, contains all properties needed for serialization
		*/
		struct Shader_GL_Header {
			unsigned int binaryFormat;
			int programBinarySize;
		};

		
		bool ShaderProgram_GL::loadProgramBinaryFromMemory(unsigned char* data, size_t size)
		{
			auto header = reinterpret_cast<Shader_GL_Header*>(data);
			void* shaderBinary = header + sizeof(Shader_GL_Header);

			// load the binary into the program object
			glProgramBinary(m_programId, header->binaryFormat, shaderBinary, header->programBinarySize);

			GLint success = 0;
			glGetProgramiv(m_programId, GL_LINK_STATUS, &success);

			if (!success) {
				// Something must have changed since the program binaries
				// were cached away.  Fallback to source shader loading path,
				// and then retrieve and cache new program binaries once again.
			}

			return (success == 1);
		}


		bool ShaderProgram_GL::loadProgramBinaryFromFile(const char* filename)
		{
			// read the program binary
			FILE* inFile = nullptr;
			
			if (fopen_s(&inFile, filename, "rb")) {
				fseek(inFile, 0, SEEK_END);
				auto size = ftell(inFile);
				auto dataPtr = std::make_unique<unsigned char[]>(size);
				
				// move this part to deserialize function, switch to istream file
				fseek(inFile, 0, SEEK_SET);
				fread((void*)dataPtr.get(), 1, size, inFile);
				fclose(inFile);

				return loadProgramBinaryFromMemory(dataPtr.get(), size);
			}
			return false;
		}
		

		bool ShaderProgram_GL::writeProgramBinaryFile(const char* filename) const
		{
			if (m_programId == 0) {
				return false;
			}

			// get the program binary
			GLint binaryLength = 0;
			GLenum binaryFormat = 0;
			glGetProgramiv(m_programId, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
			auto binary = std::make_unique<char[]>(binaryLength);
			glGetProgramBinary(m_programId, binaryLength, nullptr, &binaryFormat, (void*)binary.get());

			// write the program binary file
			FILE* outFile = nullptr;
			if (!fopen_s(&outFile, filename, "wb")) {
				return false;
			}

			// move this part to serialize function, switch to ostream file
			Shader_GL_Header header{};
			header.binaryFormat = binaryFormat;
			header.programBinarySize = binaryLength;

			fwrite(&header, sizeof(Shader_GL_Header), 1, outFile);
			fwrite(binary.get(), 1, binaryLength, outFile);
			fclose(outFile);

			return true;
		}
		
		
		void ShaderProgram_GL::deserialize(std::istream& in)
		{}


		void ShaderProgram_GL::serialize(std::ostream& out)
		{}


		void ShaderProgram_GL::useProgram() const
		{
			glUseProgram(m_programId);
		}

	}
}