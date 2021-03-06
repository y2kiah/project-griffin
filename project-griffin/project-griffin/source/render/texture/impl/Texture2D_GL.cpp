#include "../Texture2D_GL.h"
#include "../dds.h"
#include <utility/debug.h>
#include <GL/glew.h>
#include <cmath>
#include <utility/Logger.h>
//#include <SOIL.h>

//#ifdef _DEBUG
//#pragma comment( lib, "SOIL_d.lib" )
//#else
//#pragma comment( lib, "SOIL.lib" )
//#endif

namespace griffin {
	namespace render {

		// Global variables

		/**
		* Table of texture pixel formats
		* first dimension is RGBA vs. BGRA
		* second dimension is # components
		*/
		const GLenum g_formatLookup[2][4] = { { GL_RED, GL_RG, GL_RGB, GL_RGBA }, { 0, 0, GL_BGR, GL_BGRA } };

		/**
		* Table of internal formats
		* first dimension is # components
		* second dimension is component size
		* third dimension is data type
		*/
		const GLint g_internalFormatLookup[4][3][5] = {
			{
			/* R ----- standard ---- float --------- int ----------- uint ----------- sRGB -------------------- */
			/* 8  */ { { GL_R8 },    { 0 },          { GL_R8I },     { GL_R8UI },     { GL_SLUMINANCE8 } },
			/* 16 */ { { 0 },        { GL_R16F },    { GL_R16I },    { GL_R16UI },    { 0 } },
			/* 32 */ { { 0 },        { GL_R32F },    { GL_R32I },    { GL_R32UI },    { 0 } }
			},
			{
			/* RG ---- standard ---- float --------- int ----------- uint ----------- sRGB -------------------- */
			/* 8  */ { { GL_RG8 },   { 0 },          { GL_RG8I },    { GL_RG8UI },    { GL_SLUMINANCE8_ALPHA8 } },
			/* 16 */ { { 0 },        { GL_RG16F },   { GL_RG16I },   { GL_RG16UI },   { 0 } },
			/* 32 */ { { 0 },        { GL_RG32F },   { GL_RG32I },   { GL_RG32UI },   { 0 } }
			},
			{
			/* RGB --- standard ---- float --------- int ----------- uint ----------- sRGB -------------------- */
			/* 8  */ { { GL_RGB8 },  { 0 },          { GL_RGB8I },   { GL_RGB8UI },   { GL_SRGB8 } },
			/* 16 */ { { 0 },        { GL_RGB16F },  { GL_RGB16I },  { GL_RGB16UI },  { 0 } },
			/* 32 */ { { 0 },        { GL_RGB32F },  { GL_RGB32I },  { GL_RGB32UI },  { 0 } }
			},
			{
			/* RGBA -- standard ---- float --------- int ----------- uint ----------- sRGB -------------------- */
			/* 8  */ { { GL_RGBA8 }, { 0 },          { GL_RGBA8I },  { GL_RGBA8UI },  { GL_SRGB8_ALPHA8 } },
			/* 16 */ { { 0 },        { GL_RGBA16F }, { GL_RGBA16I }, { GL_RGBA16UI }, { 0 } },
			/* 32 */ { { 0 },        { GL_RGBA32F }, { GL_RGBA32I }, { GL_RGBA32UI }, { 0 } }
			}
		};


		// class Texture2D_GL

		Texture2D_GL::Texture2D_GL(Texture2D_GL&& other)
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

		Texture2D_GL::~Texture2D_GL()
		{
			if (m_glTexture != 0) {
				glDeleteTextures(1, &m_glTexture);
			}
		}

		void Texture2D_GL::setTextureParameters()
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			if (m_numMipmaps > 0) {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, m_numMipmaps);
			}
			else {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			
			// TODO: TEMP, do NOT keep this here as it will be queried at runtime, move to OpenGL setup and set a flag
			if (glewIsExtensionSupported("GL_EXT_texture_filter_anisotropic")) {
				float fLargest = 1.0f;
				glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
			}

			ASSERT_GL_ERROR;
		}

		bool Texture2D_GL::createFromMemory(unsigned char* data, size_t size, int width, int height,
											uint8_t components, uint8_t componentSize, uint8_t levels,
											uint8_t flags, unsigned int format)
		{
			assert(components > 0 && components <= 4 && "components out of range");
			assert(componentSize > 0 && componentSize <= 8 && "componentSize out of range");
			assert(levels > 0 && levels <= 13 && "level (probably) out of range, need more than 13 mipmaps?");

			m_sizeBytes = size;
			m_width = width;
			m_height = height;
			m_components = components;
			m_numMipmaps = levels - 1;

			// get number of mipmaps to be generated
			if (levels == 1 && (flags & Texture2DFlag_GenerateMipmaps)) {
				m_numMipmaps = static_cast<int>(floorf(log2f(static_cast<float>(width > height ? width : height))));
			}

			int componentsLookup = components - 1;
			int componentSizeLookup = componentSize / 2; // 1 => 0,  2 => 1,  4 => 2
			int typeLookup = (flags & Texture2DFlag_Float ? 1 :
							  (flags & Texture2DFlag_Int   ? 2 :
							   (flags & Texture2DFlag_UInt  ? 3 :
							    (flags & Texture2DFlag_sRGB  ? 4 : 0))));

			if (format == 0) {
				format = g_formatLookup[(flags & Texture2DFlag_BGRA) ? 1 : 0][componentsLookup];
			}
			GLint internalFormat = g_internalFormatLookup[componentsLookup][componentSizeLookup][typeLookup];
			
			if (format == 0 || internalFormat == 0) {
				logger.warn(Logger::Category_Render, "invalid texture format\n");
				return false;
			}

			// create texture storage
			glGenTextures(1, &m_glTexture);
			glBindTexture(GL_TEXTURE_2D, m_glTexture);
			glTexStorage2D(GL_TEXTURE_2D, m_numMipmaps + 1, internalFormat, width, height);

			// upload pixel data
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);

			if (levels == 1 && m_numMipmaps > 0) {
				glGenerateTextureMipmap(m_glTexture);
			}
			else if (m_numMipmaps > 0) {
				unsigned char* pData = data + (width * height * components * componentSize);
				int mipWidth = width / 2;
				int mipHeight = height / 2;

				for (int level = 1; level <= m_numMipmaps; ++level) {
					glTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, width, height, format, GL_UNSIGNED_BYTE, pData);
					pData += (mipWidth * mipHeight * components * componentSize);
					mipWidth /= 2;
					mipHeight /= 2;
				}

				assert(width == 1 && height == 1 && data - pData == size && "problem with mipmap chain data");
			}

			setTextureParameters();

			ASSERT_GL_ERROR;
			return true;
		}

		bool Texture2D_GL::loadDDSFromMemory(unsigned char* data, size_t size, bool sRGB)
		{
			// if image is already loaded, do some cleanup?

			glGenTextures(1, &m_glTexture);
			glBindTexture(GL_TEXTURE_2D, m_glTexture);
			
			DDSImage image;
			bool ok = image.loadFromMemory(data, true, sRGB) &&
					  !image.is_cubemap() && !image.is_volume() &&
					  image.upload_texture2D();

			if (ok) {
				m_sizeBytes = size;
				m_width = image.get_width();
				m_height = image.get_height();
				m_numMipmaps = image.get_num_mipmaps();
				m_components = image.get_components();

				setTextureParameters();

				/*
				m_glTexture = SOIL_load_OGL_texture_from_memory(
				data, static_cast<int>(size),
				SOIL_LOAD_AUTO,
				SOIL_CREATE_NEW_ID,
				SOIL_FLAG_INVERT_Y | SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_DDS_LOAD_DIRECT | SOIL_FLAG_COMPRESS_TO_DXT);
				*/

				// check for an error during the load process
				//if (m_glTexture == 0) {
				//	logger.warn(Logger::Category_Render, "SOIL loading error: %s\n", SOIL_last_result());
				//}

			}
			else {
				logger.warn(Logger::Category_Render, "texture loading error\n");
			}

			ASSERT_GL_ERROR;
			return ok;// (m_glTexture != 0);
		}

		bool Texture2D_GL::loadDDSFromInternalMemory(bool discard, bool sRGB)
		{
			bool result = loadDDSFromMemory(m_tmpData.get(), m_sizeBytes, sRGB);
			if (discard) {
				m_tmpData.reset();
			}

			return result;
		}

		bool Texture2D_GL::loadDDSFromFile(const char* filename, bool sRGB)
		{
			// if image is already loaded, do some cleanup?

			glGenTextures(1, &m_glTexture);
			glBindTexture(GL_TEXTURE_2D, m_glTexture);
			DDSImage image;
			bool ok = image.load(filename, true, sRGB) &&
					  !image.is_cubemap() && !image.is_volume() &&
					  image.upload_texture2D();

			if (ok) {
				m_sizeBytes = image.get_size();
				m_width = image.get_width();
				m_height = image.get_height();
				m_numMipmaps = image.get_num_mipmaps();
				m_components = image.get_components();

				setTextureParameters();

				/*m_glTexture = SOIL_load_OGL_texture(
					name.c_str(),
					SOIL_LOAD_AUTO,
					SOIL_CREATE_NEW_ID,
					SOIL_FLAG_INVERT_Y | SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_DDS_LOAD_DIRECT | SOIL_FLAG_COMPRESS_TO_DXT);
					*/

				// check for an error during the load process
				//if (m_glTexture == 0) {
				//	logger.warn(Logger::Category_Render, "SOIL loading error: %s\n", SOIL_last_result());
				//}
			}
			else {
				logger.warn(Logger::Category_Render, "texture loading error\n");
			}

			return ok;// (m_glTexture != 0);
		}

		void Texture2D_GL::bind(unsigned int textureSlot) const
		{
			assert(textureSlot >= 0 && textureSlot < 32 && "textureSlot must be in 0-31 range");

			glActiveTexture(GL_TEXTURE0 + textureSlot);
			glBindTexture(GL_TEXTURE_2D, m_glTexture);

			ASSERT_GL_ERROR;
		}

	}
}