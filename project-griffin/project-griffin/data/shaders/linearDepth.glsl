// VBO binding locations

#define VertexLayout_Position      0
#define VertexLayout_Normal        1
#define VertexLayout_Tangent       2
#define VertexLayout_Bitangent     3
#define VertexLayout_TextureCoords 4   // consumes up to 8 locations
#define VertexLayout_Colors        12  // consumes up to 8 locations
#define VertexLayout_CustomStart   20  // use for first custom binding location and increment


// Sampler binding locations

#define SamplerBinding_Diffuse1    4
#define SamplerBinding_Diffuse2    5
#define SamplerBinding_Diffuse3    6
#define SamplerBinding_Diffuse4    7


// Subroutine uniform locations

#define SubroutineUniform_SurfaceColor    0

uniform mat4 modelView;
uniform mat4 projection;
	
uniform float frustumNear;
uniform float frustumFar;

#ifdef _VERTEX_

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