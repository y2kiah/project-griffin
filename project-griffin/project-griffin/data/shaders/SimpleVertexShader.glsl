#version 440 core

//#define UniformLayout_ModelView           0
//#define UniformLayout_Projection          1
//#define UniformLayout_ModelViewProjection 2

#define VertexLayout_Position      0
#define VertexLayout_Normal        1
#define VertexLayout_Tangent       2
#define VertexLayout_Bitangent     3
#define VertexLayout_TextureCoords 4   // consumes up to 8 locations
#define VertexLayout_Colors        12  // consumes up to 8 locations

uniform mat4 modelViewProjection;
//layout(location = UniformLayout_ModelView) uniform mat4 modelView;
//layout(location = UniformLayout_Projection) uniform mat4 projection;
//layout(location = UniformLayout_ModelViewProjection) uniform mat4 modelViewProjection;

layout(location = VertexLayout_Position) in vec3 vertexPosition_modelspace;
layout(location = VertexLayout_Normal) in vec3 vertexNormal;
layout(location = VertexLayout_Colors) in vec4 vertexColor;
layout(location = VertexLayout_TextureCoords) in vec2 vertexUV;

out vec3 normal;
out vec4 color;
out vec2 uv;

void main() {
	gl_Position = modelViewProjection * vec4(vertexPosition_modelspace, 1.0);

	normal = vertexNormal;
	color = vertexColor;
	uv = vertexUV;
}