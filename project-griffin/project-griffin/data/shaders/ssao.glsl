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

#define SAMPLES 16 // 10 is good

uniform float totStrength = 0.35; //1.38;
uniform float strength = 0.07;
uniform float offset = 18.0;
uniform float falloff = 0.000000002; //0.000002;
uniform float rad = 0.000006; //0.006;

#ifdef _VERTEX_

	layout(location = VertexLayout_Position) in vec3 vertexPosition;

	const vec2 madd = vec2(0.5, 0.5);

	out vec2 uv;

	void main()
	{
		gl_Position = vec4(vertexPosition, 1.0);
		uv = vertexPosition.xy * madd + madd;
	}

#endif

#ifdef _FRAGMENT_
	
	uniform sampler2D normalNoise;	// 0
	uniform sampler2D colorMap;		// 1
	uniform sampler2D normalMap;	// 2
	uniform sampler2D depthMap;		// 3

	const float invSamples = 1.0 / SAMPLES;

	in vec2 uv;
	out vec4 outColor;

	float luma(vec3 color) {
		return dot(color, vec3(0.299, 0.587, 0.114));
	}

	void main()
	{
		// these are the random vectors inside a unit sphere
		const vec3 pSphere[16] = vec3[](vec3(0.53812504, 0.18565957, -0.43192),vec3(0.13790712, 0.24864247, 0.44301823),vec3(0.33715037, 0.56794053, -0.005789503),vec3(-0.6999805, -0.04511441, -0.0019965635),vec3(0.06896307, -0.15983082, -0.85477847),vec3(0.056099437, 0.006954967, -0.1843352),vec3(-0.014653638, 0.14027752, 0.0762037),vec3(0.010019933, -0.1924225, -0.034443386),vec3(-0.35775623, -0.5301969, -0.43581226),vec3(-0.3169221, 0.106360726, 0.015860917),vec3(0.010350345, -0.58698344, 0.0046293875),vec3(-0.08972908, -0.49408212, 0.3287904),vec3(0.7119986, -0.0154690035, -0.09183723),vec3(-0.053382345, 0.059675813, -0.5411899),vec3(0.035267662, -0.063188605, 0.54602677),vec3(-0.47761092, 0.2847911, -0.0271716));
		//const vec3 pSphere[8] = vec3[](vec3(0.24710192, 0.6445882, 0.033550154),vec3(0.00991752, -0.21947019, 0.7196721),vec3(0.25109035, -0.1787317, -0.011580509),vec3(-0.08781511, 0.44514698, 0.56647956),vec3(-0.011737816, -0.0643377, 0.16030222),vec3(0.035941467, 0.04990871, -0.46533614),vec3(-0.058801126, 0.7347013, -0.25399926),vec3(-0.24799341, -0.022052078, -0.13399573));
		//const vec3 pSphere[12] = vec3[](vec3(-0.13657719, 0.30651027, 0.16118456),vec3(-0.14714938, 0.33245975, -0.113095455),vec3(0.030659059, 0.27887347, -0.7332209),vec3(0.009913514, -0.89884496, 0.07381549),vec3(0.040318526, 0.40091, 0.6847858),vec3(0.22311053, -0.3039437, -0.19340435),vec3(0.36235332, 0.21894878, -0.05407306),vec3(-0.15198798, -0.38409665, -0.46785462),vec3(-0.013492276, -0.5345803, 0.11307949),vec3(-0.4972847, 0.037064247, -0.4381323),vec3(-0.024175806, -0.008928787, 0.17719103),vec3(0.694014, -0.122672155, 0.33098832));
		//const vec3 pSphere[10] = vec3[](vec3(-0.010735935, 0.01647018, 0.0062425877),vec3(-0.06533369, 0.3647007, -0.13746321),vec3(-0.6539235, -0.016726388, -0.53000957),vec3(0.40958285, 0.0052428036, -0.5591124),vec3(-0.1465366, 0.09899267, 0.15571679),vec3(-0.44122112, -0.5458797, 0.04912532),vec3(0.03755566, -0.10961345, -0.33040273),vec3(0.019100213, 0.29652783, 0.066237666),vec3(0.8765323, 0.011236004, 0.28265962),vec3(0.29264435, -0.40794238, 0.15964167));
		
		// grab a normal for reflecting the sample rays later on
		vec3 fres = normalize((textureLod(normalNoise,uv*offset, 0.0).xyz*2.0) - vec3(1.0));

		vec4 currentPixelSample = texture(normalMap,uv);

		float currentPixelDepth = texture(depthMap, uv).x;

		// current fragment coords in screen space
		vec3 ep = vec3(uv.xy,currentPixelDepth);
		// get the normal of current fragment
		vec3 norm = currentPixelSample.xyz;

		float bl = 0.0;
		// adjust for the depth (not sure if this is good)
		float radD = rad/currentPixelDepth;

		vec3 ray, se, occNorm;
		float occluderDepth, depthDifference, normDiff;

		for (int i=0; i < SAMPLES; ++i) {
			// get a vector (randomized inside of a sphere with radius 1.0) from a texture and reflect it
			ray = radD*reflect(pSphere[i],fres);

			// if the ray is outside the hemisphere then change direction
			se = ep + sign(dot(ray,norm))*ray;

			// get the depth of the occluder fragment
			vec4 occluderFragment = texture(normalMap,se.xy);
			float occluderDepth = texture(depthMap, se.xy).x; //occluderFragment.a;

			// get the normal of the occluder fragment
			occNorm = occluderFragment.xyz;

			// if depthDifference is negative = occluder is behind current fragment
			depthDifference = currentPixelDepth - occluderDepth;

			// calculate the difference between the normals as a weight

			normDiff = (1.0-dot(occNorm,norm));
			// the falloff equation, starts at falloff and is kind of 1/x^2 falling
			bl += step(falloff,depthDifference)*normDiff*(1.0-smoothstep(falloff,strength,depthDifference));
		}

		// output the result
		float ao = 1.0-totStrength*bl*invSamples;
		outColor.rgb = texture(colorMap,uv).rgb * ao;
		outColor.a = luma(outColor.rgb);
		//outColor = vec4(ao,ao,ao,0.0);
		//outColor = vec4(currentPixelDepth,currentPixelDepth,currentPixelDepth,0.0);
	}

/*
	// This is an optimized version of the same shader, but a little harder to read and understand

	#define SAMPLES 10 // 10 is good

	const float totStrength = 1.38;
	const float strength = 0.07;
	const float offset = 18.0;
	const float falloff = 0.000002;
	const float rad = 0.006;
	const float invSamples = -1.38 / SAMPLES;
	
	in vec2 uv;
	out vec3 outColor;

	void main()
	{
		// these are the random vectors inside a unit sphere
		vec3 pSphere[10] = vec3[](vec3(-0.010735935, 0.01647018, 0.0062425877),vec3(-0.06533369, 0.3647007, -0.13746321),vec3(-0.6539235, -0.016726388, -0.53000957),vec3(0.40958285, 0.0052428036, -0.5591124),vec3(-0.1465366, 0.09899267, 0.15571679),vec3(-0.44122112, -0.5458797, 0.04912532),vec3(0.03755566, -0.10961345, -0.33040273),vec3(0.019100213, 0.29652783, 0.066237666),vec3(0.8765323, 0.011236004, 0.28265962),vec3(0.29264435, -0.40794238, 0.15964167));

		// grab a normal for reflecting the sample rays later on
		vec3 fres = normalize((texture(normalNoise,uv*offset).xyz*2.0) - vec3(1.0));

		vec4 currentPixelSample = texture(normalMap,uv);

		float currentPixelDepth = currentPixelSample.a;

		// current fragment coords in screen space
		vec3 ep = vec3(uv.xy,currentPixelDepth);
		// get the normal of current fragment
		vec3 norm = currentPixelSample.xyz;
 
		float bl = 0.0;
		// adjust for the depth ( not shure if this is good..)
		float radD = rad/currentPixelDepth;

		//vec3 ray, se, occNorm;
		float occluderDepth, depthDifference;
		vec4 occluderFragment;
		vec3 ray;

		for (int i=0; i < SAMPLES; ++i) {
			// get a vector (randomized inside of a sphere with radius 1.0) from a texture and reflect it
			ray = radD * reflect(pSphere[i], fres);

			// get the depth of the occluder fragment
			occluderFragment = texture(normalMap,ep.xy + sign(dot(ray,norm) )*ray.xy);
			// if depthDifference is negative = occluder is behind current fragment
			depthDifference = currentPixelDepth-occluderFragment.a;

			// calculate the difference between the normals as a weight
			// the falloff equation, starts at falloff and is kind of 1/x^2 falling
			bl += step(falloff,depthDifference)*(1.0-dot(occluderFragment.xyz,norm))*(1.0-smoothstep(falloff,strength,depthDifference));
		}

		// output the result
		outColor = vec3(1.0+bl*invSamples, 0, 0);
	}
*/
#endif