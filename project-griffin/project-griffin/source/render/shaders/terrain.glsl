#include "source/render/shaders/layout.glsli"
#include "source/render/shaders/ubo.glsli"

#ifdef _VERTEX_
	
	layout(location = VertexLayout_Position) in vec2 vertexPosition_modelspace;

	layout(binding = SamplerBinding_Diffuse1) uniform sampler2D heightMap;

	out vec3 vPosition;

	void main() {
		vPosition.xy = vertexPosition_modelspace.xy * 64 * 1000; // 100 = horizontal scalar
		vPosition.z = texture(heightMap, vertexPosition_modelspace.xy).r * 10000.0; // 1000.0 = height scalar

	}

#endif

#ifdef _TESS_CONTROL_
	
	layout(vertices = 16) out;

	//uniform mat4 basis;
	//uniform mat4 basisTranspose;

	const /*uniform*/ float tessLevelInner = 8.0;
	const /*uniform*/ float tessLevelOuter = 16.0;

	const mat4 bicubicBasis = mat4(	1.0f/6,-3.0f/6, 3.0f/6,-1.0f/6,
									4.0f/6, 0.0f/6,-6.0f/6, 3.0f/6,
									1.0f/6, 3.0f/6, 3.0f/6,-3.0f/6,
									0.0f/6, 0.0f/6, 0.0f/6, 1.0f/6);
	
	const mat4 bicubicBasisTranspose = transpose(bicubicBasis);

	const mat4 bicubicTangentBasis = mat4(-3.0f/6,  6.0f/6,-3.0f/6, 0.0f/6,
										   0.0f/6,-12.0f/6, 9.0f/6, 0.0f/6,
										   3.0f/6,  6.0f/6,-9.0f/6, 0.0f/6,
										   0.0f/6,  0.0f/6, 3.0f/6, 0.0f/6);

	const mat4 bicubicTangentBasisTranspose = transpose(bicubicTangentBasis);
	
	in vec3 vPosition[];

	// coefficient matrices should be "patch out" variables, but crashes AMD driver (at least on my 6900)
	// to hack around the issue we use the first three elements of a normal out variable array
	//patch out mat4 cx, cy, cz;
	out mat4 cMat[]; // cMat[0] == cx, cMat[1] == cy, cMat[2] == cz, the rest are unused
	//out vec3 tcPosition[];

	void main()
	{
		//tcPosition[gl_InvocationID] = vPosition[gl_InvocationID];

		switch (gl_InvocationID) {
			case 0: {
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

				cMat[gl_InvocationID] = bicubicBasis * Px * bicubicBasisTranspose;
				break;
			}
			case 1: {
				mat4 Py = mat4(
					vPosition[0].y,  vPosition[1].y,  vPosition[2].y,  vPosition[3].y, 
					vPosition[4].y,  vPosition[5].y,  vPosition[6].y,  vPosition[7].y, 
					vPosition[8].y,  vPosition[9].y,  vPosition[10].y, vPosition[11].y, 
					vPosition[12].y, vPosition[13].y, vPosition[14].y, vPosition[15].y);

				cMat[gl_InvocationID] = bicubicBasis * Py * bicubicBasisTranspose;
				break;
			}
			case 2: {
				mat4 Pz = mat4(
					vPosition[0].z,  vPosition[1].z,  vPosition[2].z,  vPosition[3].z, 
					vPosition[4].z,  vPosition[5].z,  vPosition[6].z,  vPosition[7].z, 
					vPosition[8].z,  vPosition[9].z,  vPosition[10].z, vPosition[11].z, 
					vPosition[12].z, vPosition[13].z, vPosition[14].z, vPosition[15].z);

				cMat[gl_InvocationID] = bicubicBasis * Pz * bicubicBasisTranspose;
				break;
			}
			/*case 3: {
				mat4 Px = mat4(
					vPosition[0].x,  vPosition[1].x,  vPosition[2].x,  vPosition[3].x, 
					vPosition[4].x,  vPosition[5].x,  vPosition[6].x,  vPosition[7].x, 
					vPosition[8].x,  vPosition[9].x,  vPosition[10].x, vPosition[11].x, 
					vPosition[12].x, vPosition[13].x, vPosition[14].x, vPosition[15].x);

				cMat[gl_InvocationID] = bicubicTangentBasis * Px * bicubicTangentBasisTranspose;
				break;
			}
			case 4: {
				mat4 Py = mat4(
					vPosition[0].y,  vPosition[1].y,  vPosition[2].y,  vPosition[3].y, 
					vPosition[4].y,  vPosition[5].y,  vPosition[6].y,  vPosition[7].y, 
					vPosition[8].y,  vPosition[9].y,  vPosition[10].y, vPosition[11].y, 
					vPosition[12].y, vPosition[13].y, vPosition[14].y, vPosition[15].y);

				cMat[gl_InvocationID] = bicubicTangentBasis * Py * bicubicTangentBasisTranspose;
				break;
			}
			case 5: {
				mat4 Pz = mat4(
					vPosition[0].z,  vPosition[1].z,  vPosition[2].z,  vPosition[3].z, 
					vPosition[4].z,  vPosition[5].z,  vPosition[6].z,  vPosition[7].z, 
					vPosition[8].z,  vPosition[9].z,  vPosition[10].z, vPosition[11].z, 
					vPosition[12].z, vPosition[13].z, vPosition[14].z, vPosition[15].z);

				cMat[gl_InvocationID] = bicubicTangentBasis * Pz * bicubicTangentBasisTranspose;
				break;
			}*/
		}
	}

#endif

#ifdef _TESS_EVAL_
	
	layout(quads) in;

	layout(binding = SamplerBinding_Diffuse1) uniform sampler2D heightMap;

	//patch in mat4 cx, cy, cz;
	in mat4 cMat[];
	//in vec3 tcPosition[];

	//out vec4 tePatchDistance;
	out vec4 positionWorldspace;
	out vec4 positionViewspace;

	out vec3 normalWorldspace;
	out vec3 normalViewspace;

	out float linearDepth;
	
	out float slope;

	void main()
	{
		float u = gl_TessCoord.x;
		float v = gl_TessCoord.y;

		vec4 U = vec4(1.0, u, u*u, u*u*u);
		vec4 V = vec4(1.0, v, v*v, v*v*v);

		vec4 CVx = cMat[0] * V;
		vec4 CVy = cMat[1] * V;
		vec4 CVz = cMat[2] * V;
		
		float x = dot(CVx, U);
		float y = dot(CVy, U);
		float z = dot(CVz, U);
		
		positionWorldspace = vec4(x, y, z, 1.0);
		
		// offset surface by noise texture
		//positionWorldspace.z += texture(heightMap, vec2(x / 64.0 / 1000.0 * 8, y / 64.0 / 1000.0 * 8)).r * 1000.0f;

		// derivatives with respect to u and v
		// TODO: does not account for noise displacement of z
		vec4 dU = vec4(0.0, 1.0, 2.0*u, 3.0*u*u);
		vec4 dV = vec4(0.0, 1.0, 2.0*v, 3.0*v*v);
		float nxU = dot(CVx, dU);
		float nyU = dot(CVy, dU);
		float nzU = dot(CVz, dU);
		float nxV = dot(cMat[0] * dV, U);
		float nyV = dot(cMat[1] * dV, U);
		float nzV = dot(cMat[2] * dV, U);
		normalWorldspace = normalize(cross(vec3(nxU, nyU, nzU), vec3(nxV, nyV, nzV)));

		//tePatchDistance = vec4(u, v, 1.0-u, 1.0-v);
		
		/////
		/*float u = gl_TessCoord.x;
		float v = gl_TessCoord.y;
		vec3 x = mix(tcPosition[0], tcPosition[3], u);
		vec3 y = mix(tcPosition[12], tcPosition[15], u);
		vec4 positionWorldspace = vec4(mix(x, y, v), 1.0);*/
		/////

		// Get the position and normal in viewspace
		positionViewspace = modelView * positionWorldspace;
		normalViewspace = normalize(vec3(normalMatrix * vec4(normalWorldspace, 0.0)));

		// Get dot product between surface normal and geocentric normal for slope
		vec4 geocentricNormal = vec4(0,0,1.0,0); // simplified for now as straight up z-axis, eventually needs to be vector to center of planet
		vec3 geocentricNormalViewspace = normalize(vec3(normalMatrix * geocentricNormal));
		slope = 1.0 - dot(normalViewspace, geocentricNormalViewspace);

		// linear depth for z buffer
		linearDepth = (-positionViewspace.z - frustumNear) * inverseFrustumDistance; // map near..far linearly to 0..1

		gl_Position = modelViewProjection * positionWorldspace;
	}

#endif

#ifdef _FRAGMENT_

	// TODO: these are duplicated
	struct Light {
		vec4 positionViewspace;		// Light position in viewspace
		vec3 directionViewspace;	// Light spot direction in viewspace
		vec3 La;					// light color ambient
		vec3 Lds;					// light color diffuse and specular
		float Kc;					// attenuation base constant
		float Kl;					// attenuation linear constant
		float Kq;					// attenuation quadratic constant
		float spotAngleCutoff;		// spotlight angle cutoff, dot product comparison (% from 90deg)
		float spotEdgeBlendPct;		// spotlight edge blend, in % of spot radius
	};

	struct Material {
		vec3 Ma;					// Ambient reflectivity
		vec3 Md;					// Diffuse reflectivity
		vec3 Ms;					// Specular reflectivity
		vec3 Me;					// Emissive reflectivity
		float shininess;			// Specular shininess factor
		float metallic;				// Metallic determines color of specular highlight
	};

	Light light;
	Material material;

	// Globals
	vec3 surfaceColor;

	// Input Variables

	in vec4 positionWorldspace;
	in vec4 positionViewspace;

	in vec3 normalWorldspace;
	in vec3 normalViewspace;
	
	in float linearDepth;
	
	in float slope;

	//in vec4 color;
	//in vec2 uv;
	//in vec3 tangent;

	// Output Variables

	layout(location = 0) out vec4 albedoDisplacement;
	layout(location = 1) out vec4 eyeSpacePosition;
	layout(location = 2) out vec4 normalReflectance;

	layout(binding = SamplerBinding_Diffuse2) uniform sampler2D diffuse1;
	layout(binding = SamplerBinding_Diffuse3) uniform sampler2D diffuse2;

	// Functions

	#include "source/render/shaders/blinnPhong.glsl"

	// the following two functions are from http://www.gamasutra.com/blogs/AndreyMishkinis/20130716/196339/Advanced_Terrain_Texture_Splatting.php

	// this blends without per-vertex alpha mask
	/*float3 blend(float4 texture1, float a1, float4 texture2, float a2)
	{
		return texture1.a + a1 > texture2.a + a2 ? texture1.rgb : texture2.rgb;
	}*/

	// this blends with a per-vertex alpha mask
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
		// all TEMP for light
		vec4 lightPos = vec4( 0.1, 1.0, 150.0, 1.0 );
		vec4 lightDir = normalize(vec4( 0.0, 1.5, -1.0, 0.0 ));
		light.positionViewspace = modelView * lightPos;
		light.directionViewspace = normalize(vec3(modelView * lightDir));
				
		light.La = vec3( 0.10, 0.10, 0.10 );
		light.Lds = vec3( 0.6, 0.5, 0.4 );
		light.Kc = 1.0;
		light.Kl = 0.007;
		light.Kq = 0.0002;
		light.spotAngleCutoff = 0.96;
		light.spotEdgeBlendPct = 0.4;

		material.Ma = vec3( 1.0 );
		material.Md = vec3( 1.0 );
		material.Ms = vec3( 0.0 );
		material.Me = vec3( 0.0 );
		material.shininess = 0.0;
		material.metallic = 0.0;
		/////

		//vec4 d1 = texture(diffuse1, positionWorldspace.xy / 64.0 / 1000.0 * 8.0);
		//vec4 d2 = texture(diffuse2, positionWorldspace.xy / 64.0 / 1000.0 * 8.0);
		//vec4 surfaceColor = mix(d2, d1, smoothstep(0.2, 0.4, slope));

		// tri-planar mapping
		vec3 uvw = positionWorldspace.xyz / 64.0 / 1000.0 * 8.0;

		vec3 blending = abs(normalWorldspace);
		blending = normalize(max(blending, 0.0000001)); // Force weights to sum to 1.0
		float b = 1.0 / (blending.x + blending.y + blending.z);
		blending *= vec3(b);
		
		vec4 xaxis = texture(diffuse1, uvw.yz);
		vec4 yaxis = texture(diffuse1, uvw.xz);
		vec4 zaxis = texture(diffuse2, uvw.xy);
		// blend the results of the 3 planar projections.
		vec4 surfaceColor = xaxis * blending.x + yaxis * blending.y + zaxis * blending.z;

		vec3 lightIntensity = blinnPhongDirectionalLight(positionViewspace, normalViewspace, light.directionViewspace, surfaceColor.rgb);
		//vec3 lightIntensity = blinnPhongDirectionalLight(positionWorldspace, normalWorldspace, lightDir.xyz, vec3(1.0)/*surfaceColor.rgb*/);

		// write to g-buffer
		albedoDisplacement = vec4(lightIntensity, 1.0);
		eyeSpacePosition = vec4(positionViewspace.xyz, 0.0);
		normalReflectance = vec4(normalViewspace, 0.0);
		gl_FragDepth = linearDepth;
	}
	
#endif