#include "../Mesh_GL.h"
#include <gl/glew.h>

#include <utility>
#include <cassert>
#include <render/ShaderProgramLayouts_GL.h>
//#include "utility/container/vector_queue.h"
#include <queue>

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define Meters_to_Feet 3.2808398950131233595800524934383f

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


		void Mesh_GL::drawMesh(int drawSetIndex) const
		{
			assert(drawSetIndex >= 0 && drawSetIndex < m_numDrawSets && "drawSetIndex out of range");
			bind(drawSetIndex);

			GLenum indexType = m_indexBuffer.getIndexType();
			DrawSet& drawSet = m_drawSets[drawSetIndex];

			glDrawRangeElements(drawSet.glPrimitiveType, drawSet.indexRangeStart, drawSet.indexRangeEnd,
								drawSet.numElements, indexType, reinterpret_cast<const GLvoid*>(drawSet.indexBaseOffset));

			//glDrawRangeElementsBaseVertex(/*GL_POINTS*/drawSet.glPrimitiveType, drawSet.indexRangeStart, drawSet.indexRangeEnd,
			//							  drawSet.numElements, indexType,
			//							  reinterpret_cast<const GLvoid*>(drawSet.indexBaseOffset), drawSet.vertexBaseOffset);
			//unbind(drawSetIndex);
		}


		void Mesh_GL::draw(int modelMatLoc, int modelViewMatLoc, int mvpMatLoc, int normalMatLoc,
						   int ambientLoc, int diffuseLoc, int specularLoc, int shininessLoc,
						   const glm::mat4& viewMat, const glm::mat4& viewProjMat/*All TEMP*/) const
		{
			glm::mat4 modelToWorld;
			// temp
			modelToWorld = glm::rotate(modelToWorld, glm::radians(0.0f), glm::vec3(0, 1.0f, 0));
			modelToWorld = glm::translate(modelToWorld, glm::vec3(0.0f, 0.0f, 50.0f));
			modelToWorld = glm::scale(modelToWorld, glm::vec3(Meters_to_Feet));
			
			glm::mat4 modelView(viewMat * modelToWorld);
			glm::mat4 mvp(viewProjMat * modelToWorld);
			glm::mat4 normalMat(glm::transpose(glm::inverse(glm::mat3(modelView))));

			glUniformMatrix4fv(modelMatLoc,     1, GL_FALSE, &modelToWorld[0][0]);
			glUniformMatrix4fv(modelViewMatLoc, 1, GL_FALSE, &modelView[0][0]);
			glUniformMatrix4fv(mvpMatLoc,       1, GL_FALSE, &mvp[0][0]);
			glUniformMatrix4fv(normalMatLoc,    1, GL_FALSE, &normalMat[0][0]);

			struct BFSQueueItem {
				uint32_t  nodeIndex;
				glm::mat4 toWorld;
			};
			// TODO: convert to vector_queue
			std::queue<BFSQueueItem> bfsQueue; // lot of memory being created and destroyed every frame, should all be pre-calculated and stored for non-animated meshes?
			//bfsQueue.reserve(m_meshScene.numNodes);

			bfsQueue.push({ 0, modelToWorld }); // push root node to start traversal

			while (!bfsQueue.empty()) {
				uint32_t nodeIndex = bfsQueue.front().nodeIndex;
				assert(nodeIndex >= 0 && nodeIndex < m_meshScene.numNodes && "node index out of range");

				const auto& node = m_meshScene.sceneNodes[bfsQueue.front().nodeIndex];
				glm::mat4 transform = bfsQueue.front().toWorld * node.transform;

				glUniformMatrix4fv(modelMatLoc, 1, GL_FALSE, &transform[0][0]);

				// draw this node's meshes
				for (uint32_t m = 0; m < node.numMeshes; ++m) {
					uint32_t drawSet = m_meshScene.meshIndices[node.meshIndexOffset + m];

					Material_GL& mat = m_materials[drawSet];
					glUniform3fv(ambientLoc, 1, &mat.ambientColor[0]);
					glUniform3fv(diffuseLoc, 1, &mat.diffuseColor[0]);
					glUniform3fv(specularLoc, 1, &mat.specularColor[0]);
					glUniform1f(shininessLoc, mat.shininess);

					drawMesh(drawSet);
				}

				// push children to traverse
				for (uint32_t c = 0; c < node.numChildren; ++c) {
					uint32_t childNodeIndex = m_meshScene.childIndices[node.childIndexOffset + c];
					
					assert(childNodeIndex >= 0 && childNodeIndex < m_meshScene.numNodes && "child node index out of range");
					assert(childNodeIndex > nodeIndex && "child node is not lower in the tree");
					
					bfsQueue.push({ childNodeIndex, transform });
				}

				bfsQueue.pop();
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
					reinterpret_cast<void*>(drawSet.vertexBaseOffset) // array buffer offset
					);
			}

			if (drawSet.vertexFlags & Vertex_Normals) {
				glEnableVertexAttribArray(VertexLayout_Normal);
				glVertexAttribPointer(
					VertexLayout_Normal,
					3, GL_FLOAT, GL_FALSE,
					drawSet.vertexSize,
					reinterpret_cast<void*>(drawSet.vertexBaseOffset + drawSet.normalOffset)
					);
			}

			if (drawSet.vertexFlags & Vertex_TangentsAndBitangents) {
				glEnableVertexAttribArray(VertexLayout_Tangent);
				glEnableVertexAttribArray(VertexLayout_Bitangent);
				glVertexAttribPointer(
					VertexLayout_Tangent,
					3, GL_FLOAT, GL_FALSE,
					drawSet.vertexSize,
					reinterpret_cast<void*>(drawSet.vertexBaseOffset + drawSet.tangentOffset)
					);
				glVertexAttribPointer(
					VertexLayout_Bitangent,
					3, GL_FLOAT, GL_FALSE,
					drawSet.vertexSize,
					reinterpret_cast<void*>(drawSet.vertexBaseOffset + drawSet.bitangentOffset)
					);
			}

			if (drawSet.vertexFlags & Vertex_TextureCoords) {
				unsigned int offset = drawSet.vertexBaseOffset + drawSet.texCoordsOffset;

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
				unsigned int offset = drawSet.vertexBaseOffset + drawSet.colorsOffset;

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
		

		void Mesh_GL::unbind(int drawSetIndex) const
		{
			assert(drawSetIndex >= 0 && drawSetIndex < m_numDrawSets && "drawSetIndex out of range");

			glBindVertexArray(0); // break the existing vertex array object binding

			/*auto& drawSet = m_drawSets[drawSetIndex];

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
			}*/
		}
	}
}