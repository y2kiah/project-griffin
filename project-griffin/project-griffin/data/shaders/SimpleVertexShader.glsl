#version 440 core

#define VertexLayout_Position      0
#define VertexLayout_Normal        1
#define VertexLayout_Tangent       2
#define VertexLayout_Bitangent     3
#define VertexLayout_TextureCoords 4   // consumes up to 8 locations
#define VertexLayout_Colors        12  // consumes up to 8 locations


layout(location = VertexLayout_Position) in vec3 vertexPosition_modelspace;
layout(location = VertexLayout_Colors) in vec3 vertexColor;
layout(location = VertexLayout_TextureCoords) in vec2 vertexUV;

out vec3 color;
out vec2 uv;

void main() {
	gl_Position.xyz = vertexPosition_modelspace;
	gl_Position.w = 1.0;

	color = vertexColor;
	uv = vertexUV;
}