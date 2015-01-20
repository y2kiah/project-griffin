#include "../Mesh_GL.h"
#include <gl/glew.h>

#include <utility>


namespace griffin {
	namespace render {

		Mesh_GL::Mesh_GL(Mesh_GL&& other) :
			m_vertexBuffer(std::move(other.m_vertexBuffer)),
			m_indexBuffer(std::move(other.m_indexBuffer))
		{}

		/*void Mesh_GL::setDrawSet(int drawSetIndex, int numPrimitives, int byteOffset = 0, unsigned int primitiveType = GL_TRIANGLES)
		{
		
		}*/

		void Mesh_GL::draw(int drawSetIndex) const
		{
			GLenum indexType = m_indexBuffer.getIndexType();
			DrawSet& drawSet = m_drawSets[drawSetIndex];
			
			glDrawRangeElementsBaseVertex(drawSet.primitiveType, drawSet.indexRangeStart, drawSet.indexRangeEnd,
										  drawSet.numElements, indexType,
										  reinterpret_cast<const GLvoid*>(drawSet.indexBaseOffset), drawSet.vertexBaseOffset);
		}

	}
}