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
// UBO definitions

layout(std140) uniform CameraUniforms {
	mat4  projection;
	mat4  viewProjection;
	float frustumNear;
	float frustumFar;
	float inverseFrustumDistance;	// inverse max distance of the camera, to get linear depth
	float _pad;						// std140 is NOT CONSISTENT between AMD and Nvidia, this pad fixes it
};

layout(std140) uniform ObjectUniforms {
	mat4 modelToWorld;
	mat4 modelView;
	mat4 modelViewProjection;
	mat4 normalMatrix;
};


#ifdef _VERTEX_
	
	layout(location = VertexLayout_Position) in vec3 vertexPosition_modelspace;

	out vec3 positionModelspace;

	void main() {
		positionModelspace = vertexPosition_modelspace.xyz;
	}

#endif

#ifdef _TESS_CONTROL_
	
	layout(vertices = 16) out;

	const /*uniform*/ float tessLevelInner = 16.0;
	const /*uniform*/ float tessLevelOuter = 16.0;

	in vec3 positionModelspace[];

	out vec3 tcPosition[];

	void main()
	{
		tcPosition[gl_InvocationID] = positionModelspace[gl_InvocationID];

		if (gl_InvocationID == 0) {
			gl_TessLevelInner[0] = tessLevelInner;
			gl_TessLevelInner[1] = tessLevelInner;
			gl_TessLevelOuter[0] = tessLevelOuter;
			gl_TessLevelOuter[1] = tessLevelOuter;
			gl_TessLevelOuter[2] = tessLevelOuter;
			gl_TessLevelOuter[3] = tessLevelOuter;
		}
	}

#endif

#ifdef _TESS_EVAL_
	
	layout(quads) in;

	uniform mat4 basis;
	uniform mat4 basisTranspose;

	in vec3 tcPosition[];
	
	out vec4 tePatchDistance;
	out vec4 positionViewspace;
	out vec3 normalViewspace;
	out float linearDepth;

	void main()
	{
		float u = gl_TessCoord.x;
		float v = gl_TessCoord.y;

		mat4 Px = mat4(
			tcPosition[0].x,  tcPosition[1].x,  tcPosition[2].x,  tcPosition[3].x, 
			tcPosition[4].x,  tcPosition[5].x,  tcPosition[6].x,  tcPosition[7].x, 
			tcPosition[8].x,  tcPosition[9].x,  tcPosition[10].x, tcPosition[11].x, 
			tcPosition[12].x, tcPosition[13].x, tcPosition[14].x, tcPosition[15].x);

		mat4 Py = mat4(
			tcPosition[0].y,  tcPosition[1].y,  tcPosition[2].y,  tcPosition[3].y, 
			tcPosition[4].y,  tcPosition[5].y,  tcPosition[6].y,  tcPosition[7].y, 
			tcPosition[8].y,  tcPosition[9].y,  tcPosition[10].y, tcPosition[11].y, 
			tcPosition[12].y, tcPosition[13].y, tcPosition[14].y, tcPosition[15].y);

		mat4 Pz = mat4(
			tcPosition[0].z,  tcPosition[1].z,  tcPosition[2].z,  tcPosition[3].z, 
			tcPosition[4].z,  tcPosition[5].z,  tcPosition[6].z,  tcPosition[7].z, 
			tcPosition[8].z,  tcPosition[9].z,  tcPosition[10].z, tcPosition[11].z, 
			tcPosition[12].z, tcPosition[13].z, tcPosition[14].z, tcPosition[15].z);

		mat4 cx = basis * Px * basisTranspose;
		mat4 cy = basis * Py * basisTranspose;
		mat4 cz = basis * Pz * basisTranspose;

		vec4 U = vec4(u*u*u, u*u, u, 1.0);
		vec4 V = vec4(v*v*v, v*v, v, 1.0);

		float x = dot(cx * V, U);
		float y = dot(cy * V, U);
		float z = dot(cz * V, U);
		
		vec4 tePosition = vec4(x, y, z, 1.0);

		tePatchDistance = vec4(u, v, 1.0-u, 1.0-v);

		// Get the position and normal in viewspace
		positionViewspace = modelView * tePosition;
		normalViewspace = vec3(1.0, 1.0, 1.0);
		//normalViewspace = normalize(normalMatrix * vec4(vertexNormal, 0.0)).xyz;
		
		linearDepth = (-positionViewspace.z - frustumNear) * inverseFrustumDistance; // map near..far linearly to 0..1

		gl_Position = modelViewProjection * tePosition;
	}

#endif

#ifdef _FRAGMENT_
	
	// Globals
	vec3 surfaceColor;

	// Input Variables

	in vec4 positionViewspace;
	in vec3 normalViewspace;
	in float linearDepth;

	//in vec4 color;
	//in vec2 uv;
	//in vec3 tangent;

	// Output Variables

	layout(location = 0) out vec4 albedoDisplacement;
	layout(location = 1) out vec4 eyeSpacePosition;
	layout(location = 2) out vec4 normalReflectance;
	

	/*float3 blend(float4 texture1, float a1, float4 texture2, float a2)
	{
		return texture1.a + a1 > texture2.a + a2 ? texture1.rgb : texture2.rgb;
	}*/

	/*float3 blend(float4 texture1, float a1, float4 texture2, float a2)
	{
		float depth = 0.2;
		float ma = max(texture1.a + a1, texture2.a + a2) - depth;

		float b1 = max(texture1.a + a1 - ma, 0);
		float b2 = max(texture2.a + a2 - ma, 0);

		return (texture1.rgb * b1 + texture2.rgb * b2) / (b1 + b2);
	}*/

	void main()
	{
		surfaceColor = vec3(1.0, 0.0, 0.0); // normalViewspace; // TEMP

		albedoDisplacement = vec4(surfaceColor, 0.0);
		eyeSpacePosition = vec4(positionViewspace.xyz, 0.0);
		normalReflectance = vec4(normalViewspace, 0.0);
		gl_FragDepth = linearDepth;
	}
	
#endif