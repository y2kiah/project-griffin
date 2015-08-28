#pragma once
#ifndef GRIFFIN_TEXTURE2D_GL_H_
#define GRIFFIN_TEXTURE2D_GL_H_

#include <memory>

namespace griffin {
	namespace render {

		class Texture2D_GL {
		public:
			explicit Texture2D_GL() = default;
			explicit Texture2D_GL(std::unique_ptr<unsigned char[]> data, size_t size);
			Texture2D_GL(Texture2D_GL&& other);
			Texture2D_GL(const Texture2D_GL&) = delete;
			~Texture2D_GL();

			/**
			* load an image from memory as a new OpenGL texture
			*/
			bool loadFromMemory(unsigned char* data, size_t size);

			/**
			* load an image from the m_tmpData pointer as a new OpenGL texture
			* @param discard	true (default) to delete the memory after upload to GPU
			*/
			bool loadFromInternalMemory(bool discard = true);

			/**
			* load an image file directly as a new OpenGL texture
			*/
			bool loadFromFile(const char *filename);

			void bind(unsigned int textureSlot) const;

			int getWidth() const { return m_width; }
			int getHeight() const { return m_height; }
			int getNumMipmaps() const { return m_numMipmaps; }
			int getComponents() const { return m_components; }

		private:
			void setFilteringMode(bool mipmaps);

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