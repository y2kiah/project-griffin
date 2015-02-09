#pragma once
#ifndef GRIFFIN_INDEX_BUFFER_GL_
#define GRIFFIN_INDEX_BUFFER_GL_

#include <cstdint>
#include <memory>


namespace griffin {
	namespace render {

		class IndexBuffer_GL {
		public:
			enum IndexBufferFlags : uint8_t {
				IndexBuffer_None  = 0,
				IndexBuffer_8bit  = 1, // <! 8bit, 16bit and 32bit are mutually exclusive
				IndexBuffer_16bit = 2, // <
				IndexBuffer_32bit = 4  // <
			};

			explicit IndexBuffer_GL() {}
			explicit IndexBuffer_GL(std::unique_ptr<unsigned char[]> data, size_t size, IndexBufferFlags flags);
			IndexBuffer_GL(IndexBuffer_GL&& other);
			IndexBuffer_GL(const IndexBuffer_GL&) = delete;
			~IndexBuffer_GL();

			IndexBufferFlags getFlags() const { return m_flags; }
			unsigned int getIndexType() const;

			bool loadFromMemory(unsigned char* data, size_t size, int sizeOfElement = sizeof(uint32_t));

			bool loadFromInternalMemory(bool discard = true);

			void bind() const;

		private:
			IndexBufferFlags m_flags = IndexBuffer_None;
			unsigned int     m_glIndexBuffer = 0; // <! gl index buffer id
			size_t           m_sizeBytes = 0;
			
			std::unique_ptr<unsigned char[]> m_tmpData = nullptr;
		};


		// Inline Functions

		inline IndexBuffer_GL::IndexBuffer_GL(std::unique_ptr<unsigned char[]> data, size_t size, IndexBufferFlags flags) :
			m_tmpData(std::move(data)),
			m_sizeBytes{ size },
			m_flags{ flags }
		{}
	}
}

#endif