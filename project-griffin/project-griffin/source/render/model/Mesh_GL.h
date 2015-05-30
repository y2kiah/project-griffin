#pragma once
#ifndef GRIFFIN_MESH_GL_H_
#define GRIFFIN_MESH_GL_H_

#include <cstdint>
#include <memory>
#include <string>
#include "render/VertexBuffer_GL.h"
#include "render/IndexBuffer_GL.h"
#include "render/material/Material_GL.h"
#include <glm/mat4x4.hpp>


namespace griffin {
	namespace render {
		
		typedef std::unique_ptr<unsigned char[]> ByteBuffer;

		enum VertexFlags : uint8_t {
			Vertex_None                  = 0,
			Vertex_Positions             = 1,
			Vertex_Normals               = 1 << 1,
			Vertex_TangentsAndBitangents = 1 << 2,
			Vertex_Colors                = 1 << 3,
			Vertex_TextureCoords         = 1 << 4
		};

		/**
		* DrawSet stores the properties to render a single sub-mesh (usually one per material)
		* within the greater mesh. It includes the information necessary to interpret the vertex
		* data within the specified range (indexed into the Mesh_GL's vertex buffer), and the set
		* of faces indexed into the Mesh_GL's index buffer. This object describes the vertex data
		* available. DrawSet bit flags are checked against Material_GL bit flag requirements to
		* ensure the mesh has the required components to be rendered. The material and DrawSet
		* combine to build the VAO for rendering, where layout attribute locations are
		* predetermined and are enabled on an as-needed basis.
		*/
		struct DrawSet {
			uint8_t		vertexFlags = 0;			//<! bits set from enum VertexFlags, checked against a material's requirements

			uint32_t	vertexSize = 0;				//<! size / stride of the vertex
			uint32_t	numElements = 0;			//<! for GL_TRIANGLES it's number of primitives * 3
			uint32_t	indexBaseOffset = 0;		//<! base offset into the index buffer
			uint32_t	indexRangeStart = 0;		//<! range low of indices into the vertex buffer, before vertexBaseOffset is added
			uint32_t	indexRangeEnd = 0;			//<! range high of indices into the vertex buffer, before vertexBaseOffset is added
			uint32_t	vertexBaseOffset = 0;		//<! base offset into the vertex buffer
			uint32_t	glPrimitiveType = 0;		//<! GL_TRIANGLES is the only mode currently supported
			uint32_t	glVAO = 0;					//<! Vertex Array Object, created during Mesh_GL initialization

			// per-vertex offsets
			// position is always at offset 0
			uint8_t		normalOffset = 0;
			uint8_t		texCoordsOffset = 0;
			uint8_t		colorsOffset = 0;
			uint8_t		tangentOffset = 0;
			uint8_t		bitangentOffset = 0;

			uint8_t		numColorChannels = 0;		//<! how many 4-byte colors are there? Up to 8 supported.
			uint8_t		numTexCoordChannels = 0;	//<! how many U, UV or UVW coordinate sets are there? Up to 8 supported.
			uint8_t		numTexCoordComponents[8];	//<! indexed by channel, how many components in the channel?
		};


		struct MeshSceneNode {
			glm::mat4	transform;
			uint32_t	parentIndex = 0;			//<! index into array of nodes
			uint32_t	numChildren = 0;
			uint32_t	childIndexOffset = 0;		//<! offset into array of child node indexes, numChildren elements belong to this node
			uint32_t	numMeshes = 0;
			uint32_t	meshIndexOffset = 0;		//<! offset into array of mesh instances, numMeshes elements belong to this node
			std::string name;
		};


		struct MeshSceneGraph {
			uint32_t	numNodes = 0;
			uint32_t	numChildIndices = 0;
			uint32_t	numMeshIndices = 0;
			uint32_t	childIndicesOffset = 0;
			uint32_t	meshIndicesOffset = 0;
			MeshSceneNode * sceneNodes = nullptr;	//<! array of nodes starting with root, in breadth-first order
			uint32_t *	childIndices = nullptr;		//<! combined array of child indices for scene nodes, each an index into sceneNodes array
			uint32_t *	meshIndices = nullptr;		//<! combined array of mesh indices for scene nodes, each an index into m_drawSets array
		};


		// Need to split loaded buffer into the drawset portion and the vb/ib portion so the latter
		// can be deallocated after it's sent to the GPU. The drawset part will remain with this
		// object. It won't work to have the resource loader give a single large buffer loaded from
		// disk. The load routine needs to split the load into two allocations, one for the vb/ib
		// and one for the drawsets.
		
		class Mesh_GL {
		public:
			// Constructors / destructor
			explicit Mesh_GL() {}
			explicit Mesh_GL(ByteBuffer data, size_t size);
			explicit Mesh_GL(size_t sizeBytes, uint16_t numDrawSets, uint16_t numMaterials,
							 uint32_t materialsOffset, uint32_t meshSceneOffset,
							 ByteBuffer modelData,
							 MeshSceneGraph&& meshScene,
							 VertexBuffer_GL&& vb, IndexBuffer_GL&& ib);
			Mesh_GL(Mesh_GL&& other);
			Mesh_GL(const Mesh_GL&) = delete;
			~Mesh_GL();

			// Functions

			/**
			* Binds the vertex + index buffers, and enables vertex attrib pointers
			*/
			void bind(int drawSetIndex) const;

			/**
			* Disables vertex attrib pointers, does not unbind the vertex + index buffers since the
			* next draw call will just bind its own buffers.
			*/
			void unbind(int drawSetIndex) const;

			/**
			* Draws a single DrawSet
			*/
			void drawMesh(int drawSetIndex) const;
			
			/**
			* Draws all DrawSets without using render queue sorting, use for debugging only
			*/
			void draw(int modelMatLoc, int modelViewMatLoc, int mvpMatLoc, int normalMatLoc,
					  int ambientLoc, int diffuseLoc, int specularLoc, int shininessLoc,
					  const glm::mat4& viewMat, const glm::mat4& viewProjMat/*All TEMP*/) const;

			/**
			* Creates the Vertex Buffer Object with OpenGL for each draw set
			*/
			void initializeVAO(int drawSetIndex) const;
			void initializeVAOs() const;

//			#ifdef GRIFFIN_TOOLS_BUILD
			/**
			* Import from another file format using assimp, used in TOOLS build only
			*/
//			bool loadWithAssimp(std::string filename);
//			#endif

		private:
			// Variables
			uint16_t		m_numDrawSets = 0;
			uint16_t		m_numMaterials = 0;

			uint32_t		m_materialsOffset = 0;	//<! offsets into m_modelData
			uint32_t		m_meshSceneOffset = 0;

			DrawSet *		m_drawSets = nullptr;
			Material_GL *	m_materials = nullptr;
			MeshSceneGraph	m_meshScene;

			VertexBuffer_GL	m_vertexBuffer;
			IndexBuffer_GL	m_indexBuffer;

			size_t			m_sizeBytes = 0;

			ByteBuffer		m_modelData = nullptr;	//<! contains data for m_drawSets, m_materials, m_meshScene.sceneNodes,
													//<! m_meshScene.childIndices, m_meshScene.meshIndices
		};


		// Inline Functions

		inline Mesh_GL::Mesh_GL(ByteBuffer data, size_t size) :
			m_modelData(std::move(data)),
			m_sizeBytes{ size }
		{}


		inline Mesh_GL::Mesh_GL(size_t sizeBytes, uint16_t numDrawSets, uint16_t numMaterials,
								uint32_t materialsOffset, uint32_t meshSceneOffset,
								ByteBuffer modelData,
								MeshSceneGraph&& meshScene,
								VertexBuffer_GL&& vb, IndexBuffer_GL&& ib) :
			m_sizeBytes{ sizeBytes },
			m_numDrawSets{ numDrawSets },
			m_numMaterials{ numMaterials },
			m_materialsOffset{ materialsOffset },
			m_meshSceneOffset{ meshSceneOffset },
			m_modelData(std::move(modelData)),
			m_meshScene{ std::forward<MeshSceneGraph>(meshScene) },
			m_vertexBuffer(std::forward<VertexBuffer_GL>(vb)),
			m_indexBuffer(std::forward<IndexBuffer_GL>(ib))
		{
			// fix up the internal pointers into m_modelData
			m_drawSets = reinterpret_cast<DrawSet*>(m_modelData.get());
			m_materials = reinterpret_cast<Material_GL*>(m_modelData.get() + m_materialsOffset);

			initializeVAOs();
		}


		inline void Mesh_GL::initializeVAOs() const
		{
			for (auto ds = 0; ds < m_numDrawSets; ++ds) {
				initializeVAO(ds);
			}
		}
	}
}

#endif