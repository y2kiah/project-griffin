#pragma once
#ifndef GRIFFIN_MESH_GL_
#define GRIFFIN_MESH_GL_

#include <cstdint>
#include <memory>
#include <string>
#include "render/VertexBuffer_GL.h"
#include "render/IndexBuffer_GL.h"
#include <glm/mat4x4.hpp>


namespace griffin {
	namespace render {
		/*
		enum MaterialFlags : uint16_t {
			Material_None        = 0,
			Material_Ambient     = 1,
			Material_Diffuse     = 1 << 1,
			Material_Specular    = 1 << 2,
			Material_Emissive    = 1 << 3,
			Material_Reflective  = 1 << 4,
			Material_Transparent = 1 << 5
		};

		struct Material_GL {
			VertexFlags   vertexRequirementFlags;
			MaterialFlags materialFlags;

		};
		*/

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
		* available, but says nothing about the material's requirements for binding this data to
		* a VAO for rendering. DrawSet bit flags are checked against Material_GL bit flag
		* requirements to ensure the mesh has the required components to be rendered. The material
		* and DrawSet combine to build the VAO for rendering, where layout attribute locations are
		* predetermined and are enabled on an as-needed basis.
		*/
		struct DrawSet {
			uint8_t		 vertexFlags;				// <! bits set from enum VertexFlags, checked against a material's requirements

			unsigned int vertexSize;				// <! size / stride of the vertex
			unsigned int numElements;				// <! for GL_TRIANGLES it's number of primitives * 3
			unsigned int indexBaseOffset;			// <! base offset into the index buffer
			unsigned int indexRangeStart;			// <! range low of indices into the vertex buffer, before vertexBaseOffset is added
			unsigned int indexRangeEnd;				// <! range high of indices into the vertex buffer, before vertexBaseOffset is added
			unsigned int vertexBaseOffset;			// <! base offset into the vertex buffer
			unsigned int glPrimitiveType;			// <! GL_TRIANGLES is the only mode currently supported

			// per-vertex offsets
			// position is always at offset 0
			uint8_t      normalOffset;
			uint8_t      texCoordsOffset;
			uint8_t      colorsOffset;
			uint8_t      tangentOffset;
			uint8_t      bitangentOffset;

			uint8_t      numColorChannels;			// <! how many 4-byte colors are there? Up to 8 supported.
			uint8_t      numTexCoordChannels;		// <! how many U, UV or UVW coordinate sets are there? Up to 8 supported.
			uint8_t      numTexCoordComponents[8];	// <! indexed by channel, how many components in the channel?
		};


		struct MeshSceneNode {
			glm::mat4 transform;
			uint32_t  parentIndex = 0;      //<! index into array of nodes
			uint32_t  numChildren = 0;
			uint32_t  childIndexOffset = 0; //<! offset into array of child node indexes, numChildren elements belong to this node
			uint32_t  numMeshes = 0;
			uint32_t  meshIndexOffset = 0;  //<! offset into array of mesh instances, numMeshes elements belong to this node
		};


		struct MeshSceneGraph {
			uint32_t        numNodes = 0;
			uint32_t        numChildIndices = 0;
			uint32_t        numMeshIndices = 0;
			MeshSceneNode * sceneNodes;   //<! array of nodes starting with root, in breadth-first order
			uint32_t *      childIndices; //<! combined array of child indices for scene nodes, each an index into sceneNodes array
			uint32_t *      meshIndices;  //<! combined array of mesh indices for scene nodes, each an index into m_drawSets array
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
			explicit Mesh_GL(std::unique_ptr<unsigned char[]> data, size_t size);
			explicit Mesh_GL(uint16_t numDrawSets, std::unique_ptr<DrawSet[]> drawSets,
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
			void draw(int drawSetIndex) const;
			
			/**
			* Draws all DrawSets without using render queue sorting, use for debugging only
			*/
			void draw() const;

//			#ifdef GRIFFIN_TOOLS_BUILD
			/**
			* Import from another file format using assimp, used in TOOLS build only
			*/
//			bool loadWithAssimp(std::string filename);
//			#endif

		private:
			// Variables
			size_t          m_sizeBytes = 0;
			uint16_t		m_numDrawSets = 0;
			DrawSet *       m_drawSets = nullptr;
			MeshSceneGraph  m_meshScene;
			VertexBuffer_GL m_vertexBuffer;
			IndexBuffer_GL  m_indexBuffer;

			std::unique_ptr<unsigned char[]> m_modelData = nullptr;
		};


		// Inline Functions

		inline Mesh_GL::Mesh_GL(std::unique_ptr<unsigned char[]> data, size_t size) :
			m_modelData(std::move(data)),
			m_sizeBytes{ size }
		{}


		inline Mesh_GL::Mesh_GL(uint16_t numDrawSets, std::unique_ptr<DrawSet[]> drawSets,
								VertexBuffer_GL&& vb, IndexBuffer_GL&& ib) :
			m_numDrawSets{ numDrawSets },
			m_vertexBuffer(std::forward<VertexBuffer_GL>(vb)),
			m_indexBuffer(std::forward<IndexBuffer_GL>(ib))
		{
			// swap the DrawSet data from the incoming unique_ptr into the internal model data
			m_modelData.reset(reinterpret_cast<unsigned char*>(drawSets.release()));

			// fix up the m_drawSet pointer to point to the stored data location
			m_drawSets = reinterpret_cast<DrawSet*>(m_modelData.get());
		}


		inline Mesh_GL::~Mesh_GL() {}
	}
}

#endif