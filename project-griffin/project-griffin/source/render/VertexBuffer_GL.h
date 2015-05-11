#pragma once
#ifndef GRIFFIN_VERTEX_BUFFER_GL_H_
#define GRIFFIN_VERTEX_BUFFER_GL_H_

#include <cstdint>
#include <memory>


namespace griffin {
	namespace render {

		/**
		* Vertex buffers do not necessarily store homogenous vertex data. There may be ranges of
		* vertices within the buffer that include different components (e.g. position, normal,
		* color(s), texture coordinate(s), tangent and bitangent). The DrawSet structure contains
		* the information necessary to interpret a range of vertex data within the buffer.
		*/
		class VertexBuffer_GL {
		public:
			explicit VertexBuffer_GL() {}
			explicit VertexBuffer_GL(std::unique_ptr<unsigned char[]> data, size_t size);
			VertexBuffer_GL(VertexBuffer_GL&& other);
			VertexBuffer_GL(const VertexBuffer_GL&) = delete;
			~VertexBuffer_GL();

			bool loadFromMemory(unsigned char* data, size_t size);

			bool loadFromInternalMemory(bool discard = true);
			
			void bind() const;

		private:
			unsigned int m_glVertexBuffer = 0; // gl vertex buffer id
			size_t       m_sizeBytes = 0;

			std::unique_ptr<unsigned char[]> m_tmpData = nullptr;
		};


		// Inline Functions

		inline VertexBuffer_GL::VertexBuffer_GL(std::unique_ptr<unsigned char[]> data, size_t size) :
			m_tmpData(std::move(data)),
			m_sizeBytes{ size }
		{}

	}
}

#endif