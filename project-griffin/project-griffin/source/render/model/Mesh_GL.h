#pragma once
#ifndef GRIFFIN_MESH_GL_
#define GRIFFIN_MESH_GL_

#include <cstdint>
#include "render/VertexBuffer_GL.h"
#include "render/IndexBuffer_GL.h"


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

		enum VertexLayoutLocation : uint8_t {
			VertexLayout_Position      = 0,
			VertexLayout_Normal        = 1,
			VertexLayout_Tangent       = 2,
			VertexLayout_Bitangent     = 3,
			VertexLayout_TextureCoords = 4,  // consumes up to 8 locations
			VertexLayout_Colors        = 12  // consumes up to 8 locations
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
			VertexFlags  vertexFlags;				// <! bits set for each component included, checked against a material's requirements

			unsigned int numElements;				// <! for GL_TRIANGLES it's number of primitives * 3
			unsigned int indexBaseOffset;			// <! base offset into the index buffer
			unsigned int indexRangeStart;			// <! range low of indices into the vertex buffer, before vertexBaseOffset is added
			unsigned int indexRangeEnd;				// <! range high of indices into the vertex buffer, before vertexBaseOffset is added
			unsigned int vertexBaseOffset;			// <! base offset into the vertex buffer
			unsigned int primitiveType;				// <! GL_TRIANGLES is the only mode currently supported
			
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

		class Mesh_GL {
		public:
			// Constructors / destructor
			explicit Mesh_GL() {}
			Mesh_GL(Mesh_GL&& other);
			Mesh_GL(const Mesh_GL&) = delete;
			~Mesh_GL() {}

			// Functions

			//void setDrawSet(int drawSetIndex, int numPrimitives, int byteOffset, unsigned int primitiveType);

			void draw(int drawSetIndex) const;

			//void setVertexBuffer(RenderBufferUniquePtr &vb)	{ m_vertexBuffer = std::move(vb); }
			//void setIndexBuffer(RenderBufferUniquePtr &ib)	{ m_indexBuffer = std::move(ib); }
			//void addDrawSet(const DrawSet &ds)				{ m_drawSets.push_back(ds); }
			//void addRenderEntry(const RenderEntry &re)		{ m_renderEntries.push_back(re); }

		private:
			// Variables
			uint16_t		m_numDrawSets = 0;
			DrawSet *       m_drawSets = nullptr;

			VertexBuffer_GL m_vertexBuffer;
			IndexBuffer_GL  m_indexBuffer;
		};

	}
}

#endif