#pragma once
#ifndef GRIFFIN_TEXTURE2D_GL_H_
#define GRIFFIN_TEXTURE2D_GL_H_

#include <memory>

namespace griffin {
	namespace render {

		enum Texture2D_Flags : uint8_t {
			Texture2DFlag_None            = 0,
			Texture2DFlag_GenerateMipmaps = 1,  // generate mipmaps, do not use if mipmaps are included in buffer
			Texture2DFlag_BGRA            = 2,  // buffer format is ordered BGRA instead of RGBA (default)
			// internal format flags are mutually exclusive
			Texture2DFlag_Float           = 4,  // float internal format
			Texture2DFlag_Int             = 8,  // int internal format
			Texture2DFlag_UInt            = 16, // unsigned int internal format
			Texture2DFlag_sRGB            = 32  // texture in sRGB colorspace, use sRGB or sRGB_ALPHA format
		};


		/**
		* @class Texture2D_GL
		*/
		class Texture2D_GL {
		public:
			explicit Texture2D_GL() = default;
			explicit Texture2D_GL(std::unique_ptr<unsigned char[]> data, size_t size);
			Texture2D_GL(Texture2D_GL&& other);
			Texture2D_GL(const Texture2D_GL&) = delete;
			~Texture2D_GL();

			/**
			* create an image from memory as a new OpenGL texture
			* @param flags		texture creation options
			* @param format		OpenGL pixel format for incoming data (ex. GL_BGRA), 0 = auto-detect
			*/
			bool createFromMemory(unsigned char* data, size_t size, int width, int height,
								  uint8_t components = 4, uint8_t componentSize = 1, uint8_t levels = 1,
								  uint8_t flags = Texture2DFlag_None, unsigned int format = 0);

			/**
			* load a DDS image from memory as a new OpenGL texture
			*/
			bool loadDDSFromMemory(unsigned char* data, size_t size, bool sRGB = false);

			/**
			* load a DDS image from the m_tmpData pointer as a new OpenGL texture
			* @param discard	true (default) to delete the memory after upload to GPU
			*/
			bool loadDDSFromInternalMemory(bool discard = true, bool sRGB = false);

			/**
			* load a DDS image file directly as a new OpenGL texture
			*/
			bool loadDDSFromFile(const char *filename, bool sRGB = false);

			/**
			* Bind to an active texture slot (0 - 31) to be sampled from a shader
			* @param textureSlot	which texture slot to bind into, added to GL_TEXTURE0
			*/
			void bind(unsigned int textureSlot = 0) const;

			int getWidth() const { return m_width; }
			int getHeight() const { return m_height; }
			int getNumMipmaps() const { return m_numMipmaps; }
			int getComponents() const { return m_components; }
			unsigned int getGLTexture() { return m_glTexture; }

		private:
			void setTextureParameters();

			size_t			m_sizeBytes = 0;
			unsigned int	m_glTexture = 0;
			int				m_width = 0;
			int				m_height = 0;
			int				m_numMipmaps = 0;
			int				m_components = 0;

			std::unique_ptr<unsigned char[]> m_tmpData = nullptr;
		};


		// Inline Functions

		inline Texture2D_GL::Texture2D_GL(std::unique_ptr<unsigned char[]> data, size_t size) :
			m_tmpData(std::move(data)),
			m_sizeBytes{ size }
		{}
	}
}

#endif