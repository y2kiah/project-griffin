#include "../Texture2D_GL.h"
#include <gl/glew.h>
#include <SDL_log.h>
#include <SOIL.h>

namespace griffin {
	namespace render {

		Texture2D_GL::Texture2D_GL(Texture2D_GL&& other)
		{
			m_glTexture = other.m_glTexture;
			m_sizeBytes = other.m_sizeBytes;
			m_tmpData = std::move(other.m_tmpData);
			other.m_sizeBytes = 0;
			other.m_glTexture = 0;
			other.m_tmpData = nullptr;
		}

		Texture2D_GL::~Texture2D_GL()
		{
			if (m_glTexture != 0) {
				glDeleteTextures(1, &m_glTexture);
			}
		}

		bool Texture2D_GL::loadFromMemory(unsigned char* data, size_t size)
		{
			// if image is already loaded, do some cleanup?

			m_glTexture = SOIL_load_OGL_texture_from_memory(
				data, static_cast<int>(size),
				SOIL_LOAD_AUTO,
				SOIL_CREATE_NEW_ID,
				SOIL_FLAG_INVERT_Y | SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_DDS_LOAD_DIRECT | SOIL_FLAG_COMPRESS_TO_DXT);

			SDL_Log("tex: %d\n", m_glTexture);

			// check for an error during the load process
			if (m_glTexture == 0) {
				SDL_Log("SOIL loading error: %s\n", SOIL_last_result());
			}

			m_sizeBytes = size;

			return (m_glTexture != 0);
		}

		bool Texture2D_GL::loadFromInternalMemory(bool discard)
		{
			bool result = loadFromMemory(m_tmpData.get(), m_sizeBytes);
			if (discard) {
				m_tmpData.reset();
			}

			return result;
		}

		bool Texture2D_GL::loadFromFile(const std::string &name)
		{
			// if image is already loaded, do some cleanup?

			m_glTexture = SOIL_load_OGL_texture(
				name.c_str(),
				SOIL_LOAD_AUTO,
				SOIL_CREATE_NEW_ID,
				SOIL_FLAG_INVERT_Y | SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_DDS_LOAD_DIRECT | SOIL_FLAG_COMPRESS_TO_DXT);

			SDL_Log("tex: %d\n", m_glTexture);

			// check for an error during the load process
			if (m_glTexture == 0) {
				SDL_Log("SOIL loading error: %s\n", SOIL_last_result());
			}

			return (m_glTexture != 0);
		}

		void Texture2D_GL::bind(unsigned int textureSlot = GL_TEXTURE0) const
		{
			glActiveTexture(textureSlot);
			glBindTexture(GL_TEXTURE_2D, m_glTexture);
		}

	}
}