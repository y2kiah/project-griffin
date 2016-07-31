#include "../VertexBuffer_GL.h"
#include <utility/debug.h>
#include <GL/glew.h>
#include <cassert>

namespace griffin {
	namespace render {

		VertexBuffer_GL::VertexBuffer_GL(VertexBuffer_GL&& other) :
			m_sizeBytes{ other.m_sizeBytes },
			m_glVertexBuffer{ other.m_glVertexBuffer },
			m_tmpData(std::move(other.m_tmpData))
		{
			other.m_sizeBytes = 0;
			other.m_glVertexBuffer = 0;
			other.m_tmpData = nullptr;
		}


		VertexBuffer_GL::~VertexBuffer_GL()
		{
			if (m_glVertexBuffer != 0) {
				glDeleteBuffers(1, &m_glVertexBuffer);
			}
		}


		bool VertexBuffer_GL::loadFromMemory(const unsigned char* data, size_t size)
		{
			// generate the buffer
			glGenBuffers(1, &m_glVertexBuffer);

			glBindBuffer(GL_ARRAY_BUFFER, m_glVertexBuffer);

			// send data to OpenGL
			glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);

			m_sizeBytes = size;

			ASSERT_GL_ERROR;
			return (m_glVertexBuffer != 0);
		}


		bool VertexBuffer_GL::loadFromInternalMemory(bool discard)
		{
			bool result = loadFromMemory(m_tmpData.get(), m_sizeBytes);
			if (discard) {
				m_tmpData.reset();
			}

			return result;
		}


		void VertexBuffer_GL::bind() const
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_glVertexBuffer);
			ASSERT_GL_ERROR;
		}
	};

}