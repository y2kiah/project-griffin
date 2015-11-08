#include "../TextureCubeMap_GL.h"
#include "../dds.h"
#include <GL/glew.h>
#include <SDL_log.h>
//#include <SOIL.h>

//#ifdef _DEBUG
//#pragma comment( lib, "SOIL_d.lib" )
//#else
//#pragma comment( lib, "SOIL.lib" )
//#endif

namespace griffin {
	namespace render {

		TextureCubeMap_GL::TextureCubeMap_GL(TextureCubeMap_GL&& other)
		{
			m_glTexture = other.m_glTexture;
			m_sizeBytes = other.m_sizeBytes;
			m_width = other.m_width;
			m_height = other.m_height;
			m_numMipmaps = other.m_numMipmaps;
			m_components = other.m_components;
			m_tmpData = std::move(other.m_tmpData);
			other.m_sizeBytes = 0;
			other.m_glTexture = 0;
			other.m_width = other.m_height = other.m_numMipmaps = other.m_components = 0;
			other.m_tmpData = nullptr;
		}

		TextureCubeMap_GL::~TextureCubeMap_GL()
		{
			if (m_glTexture != 0) {
				glDeleteTextures(1, &m_glTexture);
			}
		}

		void TextureCubeMap_GL::setFilteringMode(bool mipmaps)
		{
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			if (mipmaps) {
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			}
			else {
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}

			// TODO: TEMP, do NOT keep this here as it will be queried at runtime, move to OpenGL setup and set a flag
			if (glewIsExtensionSupported("GL_EXT_texture_filter_anisotropic")) {
				float fLargest = 1.0f;
				glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
				glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
			}
		}

		bool TextureCubeMap_GL::loadFromMemory(unsigned char* data, size_t size, bool swapY)
		{
			// if image is already loaded, do some cleanup?

			glGenTextures(1, &m_glTexture);
			glBindTexture(GL_TEXTURE_CUBE_MAP, m_glTexture);
			DDSImage image;
			bool ok = image.loadFromMemory(data, true) &&
					  image.is_cubemap() &&
					  image.upload_textureCubemap(swapY);

			if (ok) {
				setFilteringMode(image.get_num_mipmaps() > 0);

				/*
				m_glTexture = SOIL_load_OGL_texture_from_memory(
				data, static_cast<int>(size),
				SOIL_LOAD_AUTO,
				SOIL_CREATE_NEW_ID,
				SOIL_FLAG_INVERT_Y | SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_DDS_LOAD_DIRECT | SOIL_FLAG_COMPRESS_TO_DXT);
				*/

				m_sizeBytes = size;
				m_width = image.get_width();
				m_height = image.get_height();
				m_numMipmaps = image.get_num_mipmaps();
				m_components = image.get_components();

				// check for an error during the load process
				//if (m_glTexture == 0) {
				//	SDL_Log("SOIL loading error: %s\n", SOIL_last_result());
				//}
			}
			else {
				SDL_Log("texture loading error\n");
			}

			return ok;// (m_glTexture != 0);
		}

		bool TextureCubeMap_GL::loadFromInternalMemory(bool discard, bool swapY)
		{
			bool result = loadFromMemory(m_tmpData.get(), m_sizeBytes, swapY);
			if (discard) {
				m_tmpData.reset();
			}

			return result;
		}

		bool TextureCubeMap_GL::loadFromFile(const char* filename, bool swapY)
		{
			// if image is already loaded, do some cleanup?

			glGenTextures(1, &m_glTexture);
			glBindTexture(GL_TEXTURE_CUBE_MAP, m_glTexture);
			DDSImage image;
			bool ok = image.load(filename, true) &&
					  image.is_cubemap() &&
					  image.upload_textureCubemap(swapY);

			if (ok) {
				setFilteringMode(image.get_num_mipmaps() > 0);

				m_sizeBytes = image.get_size();
				m_width = image.get_width();
				m_height = image.get_height();
				m_numMipmaps = image.get_num_mipmaps();
				m_components = image.get_components();

				/*m_glTexture = SOIL_load_OGL_texture(
				name.c_str(),
				SOIL_LOAD_AUTO,
				SOIL_CREATE_NEW_ID,
				SOIL_FLAG_INVERT_Y | SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_DDS_LOAD_DIRECT | SOIL_FLAG_COMPRESS_TO_DXT);
				*/

				// check for an error during the load process
				//if (m_glTexture == 0) {
				//	SDL_Log("SOIL loading error: %s\n", SOIL_last_result());
				//}
			}
			else {
				SDL_Log("texture loading error\n");
			}

			return ok;// (m_glTexture != 0);
		}

		void TextureCubeMap_GL::bind(unsigned int textureSlot) const
		{
			assert(textureSlot >= 0 && textureSlot < 32 && "textureSlot must be in 0-31 range");

			glActiveTexture(GL_TEXTURE0 + textureSlot);
			glBindTexture(GL_TEXTURE_CUBE_MAP, m_glTexture);
		}

	}
}