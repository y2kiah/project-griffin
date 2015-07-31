#pragma once
#ifndef GRIFFIN_INDEX_BUFFER_GL_H_
#define GRIFFIN_INDEX_BUFFER_GL_H_

#include <cstdint>
#include <memory>


namespace griffin {
	namespace render {

		class IndexBuffer_GL {
		public:
			enum IndexBufferFlags : uint8_t {
				IndexBuffer_None  = 0,
				IndexBuffer_8bit  = 1, // <! 8bit, 16bit and 32bit are mutually exclusive
				IndexBuffer_16bit = 2,
				IndexBuffer_32bit = 4
			};

			explicit IndexBuffer_GL() {}
			explicit IndexBuffer_GL(std::unique_ptr<unsigned char[]> data, size_t size, IndexBufferFlags flags);
			IndexBuffer_GL(IndexBuffer_GL&& other);
			IndexBuffer_GL(const IndexBuffer_GL&) = delete;
			~IndexBuffer_GL();

			IndexBufferFlags getFlags() const { return static_cast<IndexBufferFlags>(m_flags); }
			size_t getSize() const { return m_sizeBytes; }
			unsigned char* data() const { return m_tmpData.get(); }
			unsigned int getIndexType() const;

			/**
			* Sets the buffer, size, flags for default-constructed buffers. Does not load to GL,
			* call loadFromInternalMemory after this.
			*/
			inline void set(std::unique_ptr<unsigned char[]>&& data, size_t size, IndexBufferFlags flags);

			bool loadFromMemory(unsigned char* data, size_t size, int sizeOfElement = sizeof(uint32_t));

			bool loadFromInternalMemory(bool discard = true);

			void bind() const;

			inline static IndexBufferFlags getSizeFlag(int sizeOfElement);
			inline static int getSizeOfElement(IndexBufferFlags flags);

		private:
			size_t			m_sizeBytes = 0;
			unsigned int	m_glIndexBuffer = 0; // <! gl index buffer id
			uint8_t			m_flags = IndexBuffer_None;
			
			std::unique_ptr<unsigned char[]> m_tmpData = nullptr;
		};


		// Inline Functions

		inline IndexBuffer_GL::IndexBuffer_GL(std::unique_ptr<unsigned char[]> data, size_t size, IndexBufferFlags flags) :
			m_tmpData(std::move(data)),
			m_sizeBytes{ size },
			m_flags{ flags }
		{}

		inline void IndexBuffer_GL::set(std::unique_ptr<unsigned char[]>&& data, size_t size, IndexBufferFlags flags)
		{
			m_flags = flags;
			m_sizeBytes = size;
			m_tmpData = std::move(data);
		}

		IndexBuffer_GL::IndexBufferFlags IndexBuffer_GL::getSizeFlag(int sizeOfElement)
		{
			if (sizeOfElement == sizeof(uint8_t)) {
				return IndexBuffer_8bit;
			}
			else if (sizeOfElement == sizeof(uint16_t)) {
				return IndexBuffer_16bit;
			}
			return IndexBuffer_32bit;
		}
		
		int IndexBuffer_GL::getSizeOfElement(IndexBuffer_GL::IndexBufferFlags flags)
		{
			if (flags & IndexBuffer_8bit) {
				return sizeof(uint8_t);
			}
			else if (flags & IndexBuffer_16bit) {
				return sizeof(uint16_t);
			}
			return sizeof(uint32_t);
		}
	}
}

#endif