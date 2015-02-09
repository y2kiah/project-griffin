#include "../Mesh_GL.h"
#include <gl/glew.h>

#include <utility>
#include <cassert>
#include <render/ShaderProgramLayouts_GL.h>


namespace griffin {
	namespace render {

		Mesh_GL::Mesh_GL(Mesh_GL&& other) :
			m_numDrawSets{ other.m_numDrawSets },
			m_drawSets(std::move(other.m_drawSets)),
			m_vertexBuffer(std::move(other.m_vertexBuffer)),
			m_indexBuffer(std::move(other.m_indexBuffer))
		{
			other.m_numDrawSets = 0;
		}

		Mesh_GL::~Mesh_GL() {
			// delete the VAO objects
			for (auto ds = 0; ds < m_numDrawSets; ++ds) {
				glDeleteVertexArrays(1, &m_drawSets[ds].glVAO);
			}
		}

		void Mesh_GL::bind(int drawSetIndex) const
		{
			glBindVertexArray(m_drawSets[drawSetIndex].glVAO);
		}
		
		void Mesh_GL::unbind(int drawSetIndex) const
		{
			assert(drawSetIndex >= 0 && drawSetIndex < m_numDrawSets && "drawSetIndex out of range");

			auto& drawSet = m_drawSets[drawSetIndex];

			if (drawSet.vertexFlags & Vertex_Positions) {
				glDisableVertexAttribArray(VertexLayout_Position);
			}
			
			if (drawSet.vertexFlags & Vertex_Normals) {
				glDisableVertexAttribArray(VertexLayout_Normal);
			}
			
			if (drawSet.vertexFlags & Vertex_TangentsAndBitangents) {
				glDisableVertexAttribArray(VertexLayout_Tangent);
				glDisableVertexAttribArray(VertexLayout_Bitangent);
			}

			if (drawSet.vertexFlags & Vertex_TextureCoords) {
				for (int c = 0; c < drawSet.numTexCoordChannels; ++c) {
					glDisableVertexAttribArray(VertexLayout_TextureCoords + c);
				}
			}

			if (drawSet.vertexFlags & Vertex_Colors) {
				for (int c = 0; c < drawSet.numColorChannels; ++c) {
					glDisableVertexAttribArray(VertexLayout_Colors + c);
				}
			}
		}


		void Mesh_GL::draw(int drawSetIndex) const
		{
			assert(drawSetIndex >= 0 && drawSetIndex < m_numDrawSets && "drawSetIndex out of range");
			bind(drawSetIndex);

			GLenum indexType = m_indexBuffer.getIndexType();
			DrawSet& drawSet = m_drawSets[drawSetIndex];
			
			glDrawRangeElementsBaseVertex(drawSet.glPrimitiveType, drawSet.indexRangeStart, drawSet.indexRangeEnd,
										  drawSet.numElements, indexType,
										  reinterpret_cast<const GLvoid*>(drawSet.indexBaseOffset), drawSet.vertexBaseOffset);
		}


		void Mesh_GL::draw() const
		{
			for (auto ds = 0; ds < m_numDrawSets; ++ds) {
				draw(ds);
			}
		}


		void Mesh_GL::initializeVAO(int drawSetIndex) const
		{
			assert(drawSetIndex >= 0 && drawSetIndex < m_numDrawSets && "drawSetIndex out of range");

			auto& drawSet = m_drawSets[drawSetIndex];

			glGenVertexArrays(1, &drawSet.glVAO);
			glBindVertexArray(m_drawSets[drawSetIndex].glVAO);

			m_vertexBuffer.bind();
			m_indexBuffer.bind();

			if (drawSet.vertexFlags & Vertex_Positions) {
				glEnableVertexAttribArray(VertexLayout_Position);
				glVertexAttribPointer(
					VertexLayout_Position,
					3,                         // size
					GL_FLOAT,                  // type
					GL_FALSE,                  // normalize fixed-point?
					drawSet.vertexSize,        // stride
					reinterpret_cast<void*>(0) // array buffer offset
					);
			}

			if (drawSet.vertexFlags & Vertex_Normals) {
				glEnableVertexAttribArray(VertexLayout_Normal);
				glVertexAttribPointer(
					VertexLayout_Normal,
					3, GL_FLOAT, GL_FALSE,
					drawSet.vertexSize,
					reinterpret_cast<void*>(drawSet.normalOffset)
					);
			}

			if (drawSet.vertexFlags & Vertex_TangentsAndBitangents) {
				glEnableVertexAttribArray(VertexLayout_Tangent);
				glEnableVertexAttribArray(VertexLayout_Bitangent);
				glVertexAttribPointer(
					VertexLayout_Tangent,
					3, GL_FLOAT, GL_FALSE,
					drawSet.vertexSize,
					reinterpret_cast<void*>(drawSet.tangentOffset)
					);
				glVertexAttribPointer(
					VertexLayout_Bitangent,
					3, GL_FLOAT, GL_FALSE,
					drawSet.vertexSize,
					reinterpret_cast<void*>(drawSet.bitangentOffset)
					);
			}

			if (drawSet.vertexFlags & Vertex_TextureCoords) {
				int offset = drawSet.texCoordsOffset;

				for (int c = 0; c < drawSet.numTexCoordChannels; ++c) {
					glEnableVertexAttribArray(VertexLayout_TextureCoords + c);
					glVertexAttribPointer(
						VertexLayout_TextureCoords + c,
						drawSet.numTexCoordComponents[c],
						GL_FLOAT, GL_FALSE,
						drawSet.vertexSize,
						reinterpret_cast<void*>(offset)
						);

					offset += (sizeof(float) * drawSet.numTexCoordComponents[c]);
				}
			}

			if (drawSet.vertexFlags & Vertex_Colors) {
				int offset = drawSet.colorsOffset;

				for (int c = 0; c < drawSet.numColorChannels; ++c) {
					glEnableVertexAttribArray(VertexLayout_Colors + c);
					glVertexAttribPointer(
						VertexLayout_Colors + c,
						4, GL_FLOAT, GL_FALSE,
						drawSet.vertexSize,
						reinterpret_cast<void*>(offset)
						);

					offset += (sizeof(float) * 4);
				}
			}
		}
		
	}
}