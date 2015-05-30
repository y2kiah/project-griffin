#pragma once
#ifndef GRIFFIN_TEXTURE2D_GL_H_
#define GRIFFIN_TEXTURE2D_GL_H_

#include <memory>
#include <string>

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

			bool loadFromInternalMemory(bool discard = true);

			/**
			* load an image file directly as a new OpenGL texture
			*/
			bool loadFromFile(const std::string &name);

			void bind(unsigned int textureSlot) const;

		private:
			unsigned int m_glTexture = 0;
			size_t m_sizeBytes = 0;

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