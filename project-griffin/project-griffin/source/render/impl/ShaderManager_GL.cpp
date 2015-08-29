#include "../ShaderManager_GL.h"
#include <sys/stat.h>
#include <SDL_log.h>

using namespace griffin;
using namespace griffin::render;

// class ShaderManager_GL

ShaderManager_GL::~ShaderManager_GL()
{
	if (m_shaderPrograms.capacity() > RESERVE_SHADER_PROGRAMS) {
		SDL_Log("check RESERVE_SHADER_PROGRAMS: original=%d, highest=%d", RESERVE_SHADER_PROGRAMS, m_shaderPrograms.capacity());
	}
}

uint16_t ShaderManager_GL::addShaderProgram(ShaderProgram_GL&& program)
{
	m_shaderPrograms.push_back(std::forward<ShaderProgram_GL>(program));
	
	uint16_t shaderIndex = static_cast<uint16_t>(m_shaderPrograms.size() - 1);
	m_index.push_back(ShaderIndex{ ShaderKey{}, shaderIndex });
	
	return shaderIndex;
}


uint16_t ShaderManager_GL::ensureUbershaderForKey(ShaderKey key)
{
	// force to ubershader key, whether or not it was passed in that way
	key.isUbershader = 1;

	for (int s = 0; s < m_index.size(); ++s) {
		if (key.value == m_index[s].key.value) {
			return s;
		}
	}
	ShaderProgram_GL program;

	// set compiler flags
	{
		if (key.hasFirstDiffuseMap || key.hasFirstDiffuseOpacityMap) {
			program.addPreprocessorMacro("#define _HAS_DIFFUSE_MAP");
		}
		if (key.hasSpecularMap) {
			program.addPreprocessorMacro("#define _HAS_SPECULAR_MAP");
		}
		if (key.hasNormalMap) {
			program.addPreprocessorMacro("#define _HAS_NORMAL_MAP");
			if (key.usesBumpMapping) {
				program.addPreprocessorMacro("#define _BUMP_MAPPING");
			}
		}
	}

	program.compileAndLinkProgram(m_ubershaderCode.c_str());

	m_shaderPrograms.push_back(std::forward<ShaderProgram_GL>(program));
	
	uint16_t shaderIndex = static_cast<uint16_t>(m_shaderPrograms.size() - 1);
	m_index.push_back(ShaderIndex{ key, shaderIndex });

	return shaderIndex;
}


bool ShaderManager_GL::hasUbershaderForKey(ShaderKey key, uint16_t* outIndex) const
{
	// force to ubershader key, whether or not it was passed in that way
	key.isUbershader = 1;

	for (int s = 0; s < m_index.size(); ++s) {
		if (key.value == m_index[s].key.value) {
			if (outIndex != nullptr) {
				*outIndex = m_index[s].shaderIndex;
			}
			return true;
		}
	}
	return false;
}

bool ShaderManager_GL::rebuildAllCurrentUbershaders()
{
	bool result = true;
	for (int s = 0; s < m_index.size(); ++s) {
		if (m_index[s].key.isUbershader) {
			result = result && m_shaderPrograms[m_index[s].shaderIndex].compileAndLinkProgram(m_ubershaderCode.c_str());
		}
	}
	return result;
}

void ShaderManager_GL::loadUbershaderCode(const char* filename)
{
	FILE *inFile = _fsopen(filename, "rb", _SH_DENYNO);
	if (inFile) {
		// get size
		struct _stat64 st;
		_fstat64(_fileno(inFile), &st);

		// create buffer of correct size
		auto bPtr = std::make_unique<char[]>(st.st_size);

		// read data from file
		auto buffer = reinterpret_cast<void*>(bPtr.get());
		size_t sizeRead = fread(buffer, 1, st.st_size, inFile);

		fclose(inFile);

		// check for file read error
		if (sizeRead != st.st_size) {
			throw std::runtime_error("ubershader load failed");
		}

		m_ubershaderCode.assign(bPtr.get(), st.st_size);
	}
}