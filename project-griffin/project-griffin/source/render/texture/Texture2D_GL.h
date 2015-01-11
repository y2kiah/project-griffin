#pragma once
#ifndef GRIFFIN_TEXTURE2D_GL_
#define GRIFFIN_TEXTURE2D_GL_

#include <memory>
#include <string>

namespace griffin {
	namespace render {

		class Texture2D_GL {
		public:
			explicit Texture2D_GL() = default;
			explicit Texture2D_GL(std::unique_ptr<unsigned char[]> data, size_t size) :
				m_tmpData(std::move(data)),
				m_tmpSize(size)
			{}
			Texture2D_GL(Texture2D_GL&& other);
			~Texture2D_GL();

			/**
			* load an image from memory as a new OpenGL texture
			*/
			bool loadFromMemory(unsigned char* data, size_t size);

			bool loadFromInternalMemory();

			/**
			* load an image file directly as a new OpenGL texture
			*/
			bool loadFromFile(const std::string &name);

			void bindToSampler(unsigned int texture);

		private:
			Texture2D_GL(const Texture2D_GL&) = delete;

			unsigned int m_glTexture = 0;
			
			std::unique_ptr<unsigned char[]> m_tmpData;
			size_t m_tmpSize = 0;
		};

	}
}

#endif