#pragma once
#ifndef GRIFFIN_TEXTURECUBEMAP_GL_H_
#define GRIFFIN_TEXTURECUBEMAP_GL_H_

#include <memory>

namespace griffin {
	namespace render {

		class TextureCubeMap_GL {
		public:
			explicit TextureCubeMap_GL() = default;
			explicit TextureCubeMap_GL(std::unique_ptr<unsigned char[]> data, size_t size);
			TextureCubeMap_GL(TextureCubeMap_GL&& other);
			TextureCubeMap_GL(const TextureCubeMap_GL&) = delete;
			~TextureCubeMap_GL();

			/**
			* load an image from memory as a new OpenGL texture
			*/
			bool loadDDSFromMemory(unsigned char* data, size_t size, bool swapY = false, bool sRGB = false);

			/**
			* load an image from the m_tmpData pointer as a new OpenGL texture
			* @param discard	true (default) to delete the memory after upload to GPU
			*/
			bool loadDDSFromInternalMemory(bool discard = true, bool swapY = false, bool sRGB = false);

			/**
			* load an image file directly as a new OpenGL texture
			*/
			bool loadDDSFromFile(const char *filename, bool swapY = false, bool sRGB = false);

			/**
			* Bind to an active texture slot (0 - 31) to be sampled from a shader
			* @var	textureSlot		which texture slot to bind into, added to GL_TEXTURE0
			*/
			void bind(unsigned int textureSlot = 0) const;

			int getWidth() const { return m_width; }
			int getHeight() const { return m_height; }
			int getNumMipmaps() const { return m_numMipmaps; }
			int getComponents() const { return m_components; }

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

		inline TextureCubeMap_GL::TextureCubeMap_GL(std::unique_ptr<unsigned char[]> data, size_t size) :
			m_tmpData(std::move(data)),
			m_sizeBytes{ size }
		{}
	}
}

#endif