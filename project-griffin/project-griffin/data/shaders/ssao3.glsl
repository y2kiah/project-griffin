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
	uniform sampler2D positionMap;	// 4

	in vec2 uv;
	out vec4 outColor;

	float luma(vec3 color) {
		return dot(color, vec3(0.299, 0.587, 0.114));
	}

	void main() {
		// Get input for SSAO algorithm
		vec3 fragPos = texture(positionMap, uv).xyz;
		vec3 normal = texture(normalMap, uv).rgb;
		vec3 randomVec = texture(normalNoise, uv * noiseScale).xyz;
	
		// Create TBN change-of-basis matrix: from tangent-space to view-space
		vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
		vec3 bitangent = cross(normal, tangent);
		mat3 TBN = mat3(tangent, bitangent, normal);
	
		// Iterate over the sample kernel and calculate occlusion factor
		float occlusion = 0.0;

		for (int i = 0; i < kernelSize; ++i) {
			// get sample position
			vec3 sample = TBN * samples[i]; // From tangent to view-space
			sample = fragPos + sample * radius;
		
			// project sample position (to sample texture) (to get position on screen/texture)
			vec4 offset = vec4(sample, 1.0);
			offset = projection * offset; // from view to clip-space
			offset.xyz /= offset.w; // perspective divide
			offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
		
			// get sample depth
			float sampleDepth = -texture(depthMap, offset.xy).x; // Get depth value of kernel sample
		
			// range check & accumulate
			float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
			occlusion += (sampleDepth >= sample.z ? 1.0 : 0.0) * rangeCheck;
		}
	
		occlusion = 1.0 - (occlusion / kernelSize);

		outColor.rgb = texture(colorMap, uv).rgb * occlusion;
		outColor.a = luma(outColor.rgb);
	}

#endif