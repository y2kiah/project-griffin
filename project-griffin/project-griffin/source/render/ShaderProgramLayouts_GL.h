#include <cstdint>

namespace griffin {
	namespace render {

		enum UniformLayoutLocation : uint8_t {
			UniformLayout_ModelView           = 0,
			UniformLayout_Projection          = 1,
			UniformLayout_ModelViewProjection = 2
		};
		
		enum VertexLayoutLocation : uint8_t {
			VertexLayout_Position      = 0,
			VertexLayout_Normal        = 1,
			VertexLayout_Tangent       = 2,
			VertexLayout_Bitangent     = 3,
			VertexLayout_TextureCoords = 4,  // consumes up to 8 locations
			VertexLayout_Colors        = 12  // consumes up to 8 locations
		};

	}
}