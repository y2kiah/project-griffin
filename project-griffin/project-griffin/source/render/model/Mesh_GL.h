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
		
		#define GRIFFIN_MAX_MESHSCENENODE_NAME_SIZE		64
		

		// Type Declarations

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
			uint32_t	vertexSize = 0;				//<! size / stride of the vertex
			uint32_t	numElements = 0;			//<! for GL_TRIANGLES it's number of primitives * 3
			uint32_t	indexBaseOffset = 0;		//<! base offset into the index buffer
			uint32_t	indexRangeStart = 0;		//<! range low of indices into the vertex buffer, before vertexBaseOffset is added
			uint32_t	indexRangeEnd = 0;			//<! range high of indices into the vertex buffer, before vertexBaseOffset is added
			uint32_t	vertexBaseOffset = 0;		//<! base offset into the vertex buffer
			uint32_t	glPrimitiveType = 0;		//<! GL_TRIANGLES is the only mode currently supported
			uint32_t	glVAO = 0;					//<! Vertex Array Object, created during Mesh_GL initialization

			uint8_t		vertexFlags = 0;			//<! bits set from enum VertexFlags, checked against a material's requirements

			// per-vertex offsets
			// position is always at offset 0
			uint8_t		normalOffset = 0;
			uint8_t		texCoordsOffset = 0;
			uint8_t		colorsOffset = 0;
			uint8_t		tangentOffset = 0;
			uint8_t		bitangentOffset = 0;

			uint8_t		numColorChannels = 0;		//<! how many 4-byte colors are there? Up to 8 supported.
			uint8_t		numTexCoordChannels = 0;	//<! how many U, UV or UVW coordinate sets are there? Up to 8 supported.
			uint8_t		numTexCoordComponents[GRIFFIN_MAX_MATERIAL_TEXTURES];	//<! indexed by channel, how many components in the channel?
		};
		static_assert(sizeof(DrawSet) % 4 == 0, "DrawSet size should be multiple of 4 for alignment of mesh buffer");
		static_assert(std::is_trivially_copyable<DrawSet>::value, "DrawSet must be trivially copyable for serialization");


		struct MeshSceneNode {
			glm::mat4	transform;
			uint32_t	parentIndex = 0;			//<! index into array of nodes
			uint32_t	numChildren = 0;
			uint32_t	childIndexOffset = 0;		//<! offset into array of child node indexes, numChildren elements belong to this node
			uint32_t	numMeshes = 0;
			uint32_t	meshIndexOffset = 0;		//<! offset into array of mesh instances, numMeshes elements belong to this node
			// scene node string name is stored in the metadata buffer with the same index
		};
		static_assert(sizeof(MeshSceneNode) % 4 == 0, "MeshSceneNode size should be multiple of 4 for alignment of mesh buffer");
		//static_assert(std::is_trivially_copyable<MeshSceneNode>::value, "MeshSceneNode must be trivially copyable for serialization");


		struct MeshSceneNodeMetaData {
			char		name[GRIFFIN_MAX_MESHSCENENODE_NAME_SIZE];	//<! name of the scene graph node
		};
		static_assert(sizeof(MeshSceneNodeMetaData) % 4 == 0, "MeshSceneNodeMetaData size should be multiple of 4 for alignment of mesh buffer");


		struct MeshSceneGraph {
			uint32_t	numNodes = 0;
			uint32_t	numChildIndices = 0;
			uint32_t	numMeshIndices = 0;
			uint32_t	childIndicesOffset = 0;		//<! offset in bytes to start of childIndices array data
			uint32_t	meshIndicesOffset = 0;		//<! offset in bytes to start of meshIndices array data
			uint32_t	meshMetaDataOffset = 0;		//<! offset in bytes to start of sceneNodeMetaData array data
			MeshSceneNode * sceneNodes = nullptr;	//<! array of nodes starting with root, in breadth-first order, offset is always 0 relative to start of MeshSceneGraph data
			uint32_t *	childIndices = nullptr;		//<! combined array of child indices for scene nodes, each an index into sceneNodes array
			uint32_t *	meshIndices = nullptr;		//<! combined array of mesh indices for scene nodes, each an index into m_drawSets array
			MeshSceneNodeMetaData * sceneNodeMetaData = nullptr;	//<! metaData is indexed corresponding to sceneNodes
		};


		/**
		*
		*/
		class Mesh_GL {
		public:
			// Constructors / Destructor
			explicit Mesh_GL() {}

			/**
			* Constructor used by resource loading system. The modelData buffer contains the entire
			* mesh data to be deserialized. The createResourcesFromInternalMemory function must be
			* called from the OpenGL thread after this constructor is used.
			*/
			explicit Mesh_GL(ByteBuffer data, size_t size);
			
			/**
			* Constructor used by import routine. The modelData buffer has a different layout and
			* is smaller compared to when the mesh is deserialized from disk. The internal pointers
			* are still able to be hooked up, so it works just fine. This constructor results in a
			* fully loaded and usable mesh, including initing OpenGL resources. Call this from the
			* OpenGL thread only.
			*/
			explicit Mesh_GL(size_t sizeBytes, uint16_t numDrawSets, uint16_t numMaterials,
							 uint32_t drawSetsOffset, uint32_t materialsOffset, uint32_t meshSceneOffset,
							 ByteBuffer modelData, MeshSceneGraph&& meshScene,
							 VertexBuffer_GL&& vb, IndexBuffer_GL&& ib);
			
			Mesh_GL(Mesh_GL&& other);
			Mesh_GL(const Mesh_GL&) = delete;
			~Mesh_GL();

			// Functions

			size_t getSize() const { return m_sizeBytes; }

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
					  int diffuseMapLoc,
					  const glm::mat4& viewMat, const glm::mat4& viewProjMat/*All TEMP*/) const;

			/**
			* Creates the Vertex Buffer Object with OpenGL for each draw set
			*/
			void initializeVAO(int drawSetIndex) const;
			void initializeVAOs() const;

			/**
			* Serialization functions, to/from binary stream
			*/
			void serialize(std::ostream& out);
			void deserialize(std::istream& in);

			/**
			* Sets properties and internal pointers based on data loaded into m_modelData buffer
			*/
			void loadFromInternalMemory();

			/**
			* Creates index/vertex buffers based on data loaded into m_modelData buffer. Call this
			* from the OpenGL thread after calling loadFromInternalMemory.
			*/
			void createResourcesFromInternalMemory(const std::wstring& filePath);

		private:

			// Variables
			uint32_t		m_sizeBytes = 0;			//<! contains the size of m_modelData in bytes

			uint32_t		m_numDrawSets = 0;
			uint32_t		m_numMaterials = 0;

			uint32_t		m_drawSetsOffset = 0;		//<! offsets into m_modelData
			uint32_t		m_materialsOffset = 0;
			uint32_t		m_meshSceneOffset = 0;
			uint32_t		m_vertexBufferOffset = 0;	//<! 0 when m_vertexBuffer contains internal data, > 0 when vertex data is part of m_modelData
			uint32_t		m_indexBufferOffset = 0;	//<! 0 when m_indexBuffer contains internal data, > 0 when index data is part of m_modelData

			DrawSet *		m_drawSets = nullptr;
			Material_GL *	m_materials = nullptr;

			MeshSceneGraph	m_meshScene;

			VertexBuffer_GL	m_vertexBuffer;
			IndexBuffer_GL	m_indexBuffer;

			ByteBuffer		m_modelData = nullptr;		//<! contains data for m_drawSets, m_materials, m_meshScene.sceneNodes,
														//<! m_meshScene.childIndices, m_meshScene.meshIndices
		};


		// Inline Functions

		inline void Mesh_GL::initializeVAOs() const
		{
			for (uint32_t ds = 0; ds < m_numDrawSets; ++ds) {
				initializeVAO(ds);
			}
		}
	}
}

#endif