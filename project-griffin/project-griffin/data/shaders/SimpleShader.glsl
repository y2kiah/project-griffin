//#define UniformLayout_ModelToWorld        0
//#define UniformLayout_ViewProjection      1
//#define UniformLayout_ModelViewProjection 2

#define VertexLayout_Position      0
#define VertexLayout_Normal        1
#define VertexLayout_Tangent       2
#define VertexLayout_Bitangent     3
#define VertexLayout_TextureCoords 4   // consumes up to 8 locations
#define VertexLayout_Colors        12  // consumes up to 8 locations
#define VertexLayout_CustomStart   20  // use for first custom binding location and increment

#ifdef _VERTEX_

	uniform mat4 modelToWorld;
	uniform mat4 modelView;
	uniform mat4 viewProjection;
	uniform mat4 modelViewProjection;
	uniform mat4 normalMatrix; // inverse transpose of upper-left 3x3 of modelView
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

		normal = normalize(normalMatrix * vec4(vertexNormal, 0.0)).xyz;
		color = vertexColor;
		uv = vertexUV;
	}

#endif

#ifdef _FRAGMENT_
	uniform vec3 diffuseColor;
	uniform sampler2D diffuse;

	in vec3 normal;
	in vec4 color;
	in vec2 uv;

	layout(location = 0) out vec4 diffuseDisplacement;
	layout(location = 1) out vec3 eyeSpacePosition;
	layout(location = 2) out vec4 normalReflectance;

	void main() {
		//outColor = (texture(diffuse, uv).rgb * color.rgb);
		diffuseDisplacement.rgb = diffuseColor;
		normalReflectance.rgb = (normal + 1.0) * 0.5;

		diffuseDisplacement.rgb = normalReflectance.rgb; // temp show normal
	}


	// the next two functions could be used for position channel of g-buffer in referred rendering
	// http://www.beyond3d.com/content/articles/19/2
	/*
	vec3 packPositionForGBuffer(vec4 inp)
	{
		vec3 o = inp.xyz / inp.w;
		o.xy = (o.xy * 0.5) + 0.5;
		return o;
	}

	vec4 unpackPositionFromGBuffer(vec3 inp)
	{
		// matGBufferWarp is the matrix that takes us from screen space position to view space for lighting
		vec4 o = mul(vec4(inp,1), matGBufferWarp);
		o.xyz = o.xyz / o.w;
		return o;
	}
	*/
#endif