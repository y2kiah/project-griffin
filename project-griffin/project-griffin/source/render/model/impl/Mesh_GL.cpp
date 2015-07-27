/**
* @file Mesh_GL.cpp
* @author Jeff Kiah
*/
#include <render/model/Mesh_GL.h>
#include <gl/glew.h>

#include <utility>
#include <cassert>
#include <render/ShaderProgramLayouts_GL.h>
#include "utility/container/vector_queue.h"
//#include <queue>

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define Meters_to_Feet 3.2808398950131233595800524934383f

namespace griffin {
	namespace render {

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

			glUniformMatrix4fv(modelMatLoc, 1, GL_FALSE, &modelToWorld[0][0]);
			glUniformMatrix4fv(modelViewMatLoc, 1, GL_FALSE, &modelView[0][0]);
			glUniformMatrix4fv(mvpMatLoc, 1, GL_FALSE, &mvp[0][0]);
			glUniformMatrix4fv(normalMatLoc, 1, GL_FALSE, &normalMat[0][0]);

			struct BFSQueueItem {
				uint32_t  nodeIndex;
				glm::mat4 toWorld;
			};
			// TODO: lot of memory being created and destroyed every frame, should all be pre-calculated and stored for non-animated meshes?
			vector_queue<BFSQueueItem> bfsQueue;
			bfsQueue.reserve(m_meshScene.numNodes);

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


		// Serialization Functions

		/**
		* Mesh_GL binary file header, contains all properties needed for serialization
		*/
		struct Mesh_GL_Header {
			size_t		bufferSize;
			uint16_t	numDrawSets;
			uint16_t	numMaterials;
			uint32_t	drawSetsOffset;
			uint32_t	materialsOffset;
			uint32_t	meshSceneOffset;
			uint32_t	sceneNumNodes;
			uint32_t	sceneNumChildIndices;
			uint32_t	sceneNumMeshIndices;
			uint32_t	sceneChildIndicesOffset;
			uint32_t	sceneMeshIndicesOffset;
			uint32_t	sceneMetaDataOffset;
		};

		void Mesh_GL::serialize(std::ostream& out)
		{
			uint32_t headerSize = sizeof(Mesh_GL_Header);
			uint32_t drawSetsSize = m_numDrawSets * sizeof(DrawSet);
			uint32_t materialsSize = m_numMaterials * sizeof(Material_GL);
			uint32_t sceneNodesSize = m_meshScene.numNodes * sizeof(MeshSceneNode);
			uint32_t sceneChildIndicesSize = m_meshScene.numChildIndices * sizeof(uint32_t);
			uint32_t sceneMeshIndicesSize = m_meshScene.numMeshIndices * sizeof(uint32_t);
			uint32_t sceneMetaDataSize = m_meshScene.numNodes * sizeof(MeshSceneNodeMetaData);

			// get the total size of the model buffer
			size_t bufferSize = drawSetsSize + materialsSize + sceneNodesSize +
				sceneChildIndicesSize + sceneMeshIndicesSize + sceneMetaDataSize;
				
			// build the header containing sizes and offsets
			Mesh_GL_Header header = {};
			header.bufferSize = bufferSize;
			header.numDrawSets = m_numDrawSets;
			header.numMaterials = m_numMaterials;
			header.drawSetsOffset = headerSize + 0;
			header.materialsOffset = header.drawSetsOffset + drawSetsSize;
			header.meshSceneOffset = header.materialsOffset + materialsSize;
			header.sceneNumNodes = m_meshScene.numNodes;
			header.sceneNumChildIndices = m_meshScene.numChildIndices;
			header.sceneNumMeshIndices = m_meshScene.numMeshIndices;
			header.sceneChildIndicesOffset = header.meshSceneOffset + sceneNodesSize;
			header.sceneMeshIndicesOffset = header.sceneChildIndicesOffset + sceneChildIndicesSize;
			header.sceneMetaDataOffset = header.sceneMeshIndicesOffset + sceneMeshIndicesSize;

			// write header
			out.write(reinterpret_cast<const char*>(&header), headerSize);

			// write data buffers
			out.write(reinterpret_cast<const char*>(m_drawSets), drawSetsSize);
			out.write(reinterpret_cast<const char*>(m_materials), materialsSize);
			out.write(reinterpret_cast<const char*>(m_meshScene.sceneNodes), sceneNodesSize);
			out.write(reinterpret_cast<const char*>(m_meshScene.childIndices), sceneChildIndicesSize);
			out.write(reinterpret_cast<const char*>(m_meshScene.meshIndices), sceneMeshIndicesSize);
			out.write(reinterpret_cast<const char*>(m_meshScene.sceneNodeMetaData), sceneMetaDataSize);
		}


		void Mesh_GL::deserialize(std::istream& in)
		{
			// read the header
			Mesh_GL_Header header = {};
			size_t headerSize = sizeof(Mesh_GL_Header);
			in.read(reinterpret_cast<char*>(&header), headerSize);

			// total size of the model data buffer, we are matching how the resource loader works
			size_t totalSize = headerSize + header.bufferSize;
			m_modelData = std::make_unique<unsigned char[]>(totalSize);

			// copy the header into the modelData buffer (this is how it would be using the resource loader)
			memcpy_s(m_modelData.get(), headerSize, &header, headerSize);

			// read the model data buffer
			in.read(reinterpret_cast<char*>(m_modelData.get() + headerSize), header.bufferSize);

			loadFromInternalMemory();
			createResourcesFromInternalMemory();
		}


		void Mesh_GL::loadFromInternalMemory()
		{
			// the header exists at the beginning of the modelData buffer
			Mesh_GL_Header& header = *reinterpret_cast<Mesh_GL_Header*>(m_modelData.get());
			size_t totalSize = sizeof(Mesh_GL_Header) + header.bufferSize;

			// copy properties from header to member vars
			m_sizeBytes = totalSize;
			m_numDrawSets = header.numDrawSets;
			m_numMaterials = header.numMaterials;
			m_drawSetsOffset = header.drawSetsOffset;
			m_materialsOffset = header.materialsOffset;
			m_meshSceneOffset = header.meshSceneOffset;
			m_meshScene.numNodes = header.sceneNumNodes;
			m_meshScene.numChildIndices = header.sceneNumChildIndices;
			m_meshScene.numMeshIndices = header.sceneNumMeshIndices;
			m_meshScene.childIndicesOffset = header.sceneChildIndicesOffset;
			m_meshScene.meshIndicesOffset = header.sceneMeshIndicesOffset;
			m_meshScene.meshMetaDataOffset = header.sceneMetaDataOffset;

			// fix up internal pointers based on offsets in header
			m_drawSets = reinterpret_cast<DrawSet*>(m_modelData.get() + m_drawSetsOffset);
			m_materials = reinterpret_cast<Material_GL*>(m_modelData.get() + m_materialsOffset);
			m_meshScene.sceneNodes = reinterpret_cast<MeshSceneNode*>(m_modelData.get() + m_meshSceneOffset);
			m_meshScene.childIndices = reinterpret_cast<uint32_t*>(m_modelData.get() + m_meshScene.childIndicesOffset);
			m_meshScene.meshIndices = reinterpret_cast<uint32_t*>(m_modelData.get() + m_meshScene.meshIndicesOffset);
			m_meshScene.sceneNodeMetaData = reinterpret_cast<MeshSceneNodeMetaData*>(m_modelData.get() + m_meshScene.meshMetaDataOffset);
		}

		
		void Mesh_GL::createResourcesFromInternalMemory()
		{
			// vertex buffer
			// index buffer
			initializeVAOs();

			// materials
		}


		// Constructors / Destructor

		Mesh_GL::Mesh_GL(ByteBuffer data, size_t size) :
			m_modelData(std::move(data)),
			m_sizeBytes{ size }
		{
			loadFromInternalMemory();
			createResourcesFromInternalMemory();
		}


		Mesh_GL::Mesh_GL(size_t sizeBytes, uint16_t numDrawSets, uint16_t numMaterials,
						 uint32_t drawSetsOffset, uint32_t materialsOffset, uint32_t meshSceneOffset,
						 ByteBuffer modelData,
						 MeshSceneGraph&& meshScene,
						 VertexBuffer_GL&& vb, IndexBuffer_GL&& ib) :
			m_sizeBytes{ sizeBytes },
			m_numDrawSets{ numDrawSets },
			m_numMaterials{ numMaterials },
			m_drawSetsOffset{ drawSetsOffset },
			m_materialsOffset{ materialsOffset },
			m_meshSceneOffset{ meshSceneOffset },
			m_modelData(std::move(modelData)),
			m_meshScene{ std::forward<MeshSceneGraph>(meshScene) },
			m_vertexBuffer(std::forward<VertexBuffer_GL>(vb)),
			m_indexBuffer(std::forward<IndexBuffer_GL>(ib))
		{
			// fix up the internal pointers into m_modelData
			m_drawSets = reinterpret_cast<DrawSet*>(m_modelData.get() + m_drawSetsOffset);
			m_materials = reinterpret_cast<Material_GL*>(m_modelData.get() + m_materialsOffset);

			initializeVAOs();
		}


		Mesh_GL::Mesh_GL(Mesh_GL&& other) :
			m_numDrawSets{ other.m_numDrawSets },
			m_numMaterials{ other.m_numMaterials },
			m_drawSetsOffset{ other.m_drawSetsOffset },
			m_materialsOffset{ other.m_materialsOffset },
			m_meshSceneOffset{ other.m_meshSceneOffset },
			m_drawSets{ other.m_drawSets },
			m_materials{ other.m_materials },
			m_meshScene(std::move(other.m_meshScene)),
			m_vertexBuffer(std::move(other.m_vertexBuffer)),
			m_indexBuffer(std::move(other.m_indexBuffer)),
			m_sizeBytes{ other.m_sizeBytes },
			m_modelData(std::move(other.m_modelData))
		{
			other.m_numDrawSets = 0;
			other.m_numMaterials = 0;
			other.m_drawSetsOffset = 0;
			other.m_materialsOffset = 0;
			other.m_meshSceneOffset = 0;
			other.m_drawSets = nullptr;
			other.m_materials = nullptr;
			other.m_sizeBytes = 0;
		}


		Mesh_GL::~Mesh_GL() {
			// delete the VAO objects
			for (auto ds = 0; ds < m_numDrawSets; ++ds) {
				glDeleteVertexArrays(1, &m_drawSets[ds].glVAO);
			}
		}
	}
}