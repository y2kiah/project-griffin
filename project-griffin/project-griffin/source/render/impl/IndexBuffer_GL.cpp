#include "../IndexBuffer_GL.h"
#include <gl/glew.h>


namespace griffin {
	namespace render {

		IndexBuffer_GL::IndexBuffer_GL(IndexBuffer_GL&& other)
		{
			m_flags = other.m_flags;
			m_sizeBytes = other.m_sizeBytes;
			m_glIndexBuffer = other.m_glIndexBuffer;
			m_tmpData = std::move(other.m_tmpData);
			other.m_flags = IndexBuffer_None;
			other.m_sizeBytes = 0;
			other.m_glIndexBuffer = 0;
			other.m_tmpData = nullptr;
		}


		IndexBuffer_GL::~IndexBuffer_GL()
		{
			if (m_glIndexBuffer != 0) {
				glDeleteBuffers(1, &m_glIndexBuffer);
			}
		}


		unsigned int IndexBuffer_GL::getIndexType() const
		{
			GLenum type = GL_UNSIGNED_INT;
			if (m_flags & IndexBuffer_16bit) {
				type = GL_UNSIGNED_SHORT;
			}
			else if (m_flags & IndexBuffer_8bit) {
				type = GL_UNSIGNED_BYTE;
			}
			return type;
		}


		bool IndexBuffer_GL::loadFromMemory(unsigned char* data, size_t size)
		{
			// generate the buffer
			glGenBuffers(1, &m_glIndexBuffer);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);

			// send data to OpenGL
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);

			return (m_glIndexBuffer != 0);
		}


		bool IndexBuffer_GL::loadFromInternalMemory(bool discard)
		{
			bool result = loadFromMemory(m_tmpData.get(), m_sizeBytes);
			if (discard) {
				m_tmpData.reset();
			}

			return result;
		}


		void IndexBuffer_GL::bind() const
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);
		}

	}
}