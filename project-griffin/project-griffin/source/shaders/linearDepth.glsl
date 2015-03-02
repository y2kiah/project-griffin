#ifdef _VERTEX_
	
	#define VertexLayout_Position      0
	#define VertexLayout_Normal        1
	#define VertexLayout_Tangent       2
	#define VertexLayout_Bitangent     3
	#define VertexLayout_TextureCoords 4   // consumes up to 8 locations
	#define VertexLayout_Colors        12  // consumes up to 8 locations

	uniform mat4 modelView;
	uniform mat4 projection;
	
	uniform float frustumNear;
	uniform float frustumFar;

	layout(location = VertexLayout_Position) in vec3 vertexPosition_modelspace;
	layout(location = VertexLayout_Normal) in vec3 vertexNormal;

	out float linearDepth;

	void main()
	{
		vec4 viewPos = modelView * vec4(vertexPosition_modelspace, 1.0); // vertex to eyespace
		linearDepth = (-viewPos.z - frustumNear) / (frustumFar - frustumNear); // map near..far to 0..1
		gl_Position = projection * viewPos;

		//normal = normalize(modelToWorld * vec4(vertexNormal, 0.0)).xyz;
		//color = vertexColor;
		//uv = vertexUV;
	}

#endif

#ifdef _FRAGMENT_
	
	in float linearDepth;

	out vec4 outColor;

	void main()
	{
		outColor = vec4(linearDepth, 0.0, 0.0, 1.0);
	}

#endif