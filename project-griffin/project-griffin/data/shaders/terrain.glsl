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

	out vec3 vPosition;

	void main() {
		vPosition = vertexPosition_modelspace.xyz;
	}

#endif

#ifdef _TESS_CONTROL_
	
	layout(vertices = 16) out;

	uniform mat4 basis;
	uniform mat4 basisTranspose;

	const /*uniform*/ float tessLevelInner = 8.0;
	const /*uniform*/ float tessLevelOuter = 8.0;

	in vec3 vPosition[];

	patch out mat4 cx, cy, cz;

	void main()
	{
		if (gl_InvocationID == 0) {
			gl_TessLevelInner[0] = tessLevelInner;
			gl_TessLevelInner[1] = tessLevelInner;
			gl_TessLevelOuter[0] = tessLevelOuter;
			gl_TessLevelOuter[1] = tessLevelOuter;
			gl_TessLevelOuter[2] = tessLevelOuter;
			gl_TessLevelOuter[3] = tessLevelOuter;

			// calculate coefficient matrices for cubic surface
			mat4 Px = mat4(
				vPosition[0].x,  vPosition[1].x,  vPosition[2].x,  vPosition[3].x, 
				vPosition[4].x,  vPosition[5].x,  vPosition[6].x,  vPosition[7].x, 
				vPosition[8].x,  vPosition[9].x,  vPosition[10].x, vPosition[11].x, 
				vPosition[12].x, vPosition[13].x, vPosition[14].x, vPosition[15].x);

			mat4 Py = mat4(
				vPosition[0].y,  vPosition[1].y,  vPosition[2].y,  vPosition[3].y, 
				vPosition[4].y,  vPosition[5].y,  vPosition[6].y,  vPosition[7].y, 
				vPosition[8].y,  vPosition[9].y,  vPosition[10].y, vPosition[11].y, 
				vPosition[12].y, vPosition[13].y, vPosition[14].y, vPosition[15].y);

			mat4 Pz = mat4(
				vPosition[0].z,  vPosition[1].z,  vPosition[2].z,  vPosition[3].z, 
				vPosition[4].z,  vPosition[5].z,  vPosition[6].z,  vPosition[7].z, 
				vPosition[8].z,  vPosition[9].z,  vPosition[10].z, vPosition[11].z, 
				vPosition[12].z, vPosition[13].z, vPosition[14].z, vPosition[15].z);

			cx = basis * Px * basisTranspose;
			cy = basis * Py * basisTranspose;
			cz = basis * Pz * basisTranspose;
		}
	}

#endif

#ifdef _TESS_EVAL_
	
	layout(quads) in;

	patch in mat4 cx, cy, cz;
	
	out vec4 tePatchDistance;
	out vec4 positionViewspace;
	out vec3 normalViewspace;
	out float linearDepth;

	void main()
	{
		float u = gl_TessCoord.x;
		float v = gl_TessCoord.y;

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

		/*float u = gl_TessCoord.x;
		float v = gl_TessCoord.y;
		vec3 x = mix(vPosition[0], vPosition[3], u);
		vec3 y = mix(vPosition[12], vPosition[15], u);
		vec3 tePosition = mix(x, y, v);
		gl_Position = modelViewProjection * vec4(tePosition, 1.0);*/
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