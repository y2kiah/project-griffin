#include "../ShaderManager_GL.h"

using namespace griffin;
using namespace griffin::render;

// class ShaderManager_GL

uint16_t ShaderManager_GL::addShaderProgram(ShaderProgram_GL&& program)
{
	m_shaderPrograms.push_back(std::forward<ShaderProgram_GL>(program));
	
	uint16_t shaderIndex = static_cast<uint16_t>(m_shaderPrograms.size() - 1);
	m_index.push_back(ShaderIndex{ ShaderKey{}, shaderIndex });
	
	return shaderIndex;
}


uint16_t ShaderManager_GL::ensureUbershaderForKey(ShaderKey key)
{
	for (int s = 0; s < m_index.size(); ++s) {
		if (key.value == m_index[s].key.value) {
			return s;
		}
	}
	ShaderProgram_GL program;
	program.compileAndLinkProgram(ubershadercode);

	m_shaderPrograms.push_back(std::forward<ShaderProgram_GL>(program));
	
	uint16_t shaderIndex = static_cast<uint16_t>(m_shaderPrograms.size() - 1);
	m_index.push_back(ShaderIndex{ key, shaderIndex });

	return shaderIndex;
}


bool ShaderManager_GL::hasUbershaderForKey(ShaderKey key, uint16_t* outIndex) const
{
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