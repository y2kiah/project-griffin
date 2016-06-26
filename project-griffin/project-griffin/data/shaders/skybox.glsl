// VBO binding locations

#define VertexLayout_Position      0
#define VertexLayout_Normal        1
#define VertexLayout_Tangent       2
#define VertexLayout_Bitangent     3
#define VertexLayout_TextureCoords 4   // consumes up to 8 locations
#define VertexLayout_Colors        12  // consumes up to 8 locations
#define VertexLayout_CustomStart   20  // use for first custom binding location and increment


// Subroutine uniform locations

#define SubroutineUniform_SurfaceColor		0

#ifdef _VERTEX_
	
	uniform mat4 modelViewProjection;

	layout(location = VertexLayout_Position) in vec3 vertexPosition_modelspace;

	out vec3 uvw;

	void main()
	{
		uvw = vertexPosition_modelspace;
		gl_Position = modelViewProjection * vec4(vertexPosition_modelspace, 1.0);
	}
	
#endif

#ifdef _FRAGMENT_

	uniform samplerCube cubemap; // 0
	
	in vec3 uvw;

	layout(location = 0) out vec4 outColor;

	float luma(vec3 color) {
		return dot(color, vec3(0.299, 0.587, 0.114));
	}

	void main() {
		outColor.rgb = textureLod(cubemap, uvw, 0).rgb;
		outColor.a = 0.0; //luma(outColor.rgb);
	}

#endif