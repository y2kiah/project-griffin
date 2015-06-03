#include "source/render/shaders/layout.glsli"

// #define PI 3.14159265
#define DL    2.399963229728653			// PI * (3.0 - sqrt(5.0))
#define EULER 2.718281828459045

#ifdef _VERTEX_

	layout(location = VertexLayout_Position) in vec3 vertexPosition;

	const vec2 madd = vec2(0.5, 0.5);

	out vec2 uv;

	void main() {
		gl_Position = vec4(vertexPosition, 1.0);
		uv = vertexPosition.xy * madd + madd;
	}

#endif

#ifdef _FRAGMENT_

	uniform float cameraNear;
	uniform float cameraFar;

	uniform vec2  size = ivec2(1600,900);	// texture width, height
	uniform float aoClamp = 0.5;			// depth clamp - reduces haloing at screen edges

	uniform float lumInfluence = 0.5;		// how much luminance affects occlusion
	
	uniform sampler2D normalNoise;	// 0
	uniform sampler2D colorMap;		// 1
	uniform sampler2D normalMap;	// 2
	uniform sampler2D depthMap;		// 3

	// helpers

	float width = size.x;					// texture width
	float height = size.y;					// texture height

	float cameraFarPlusNear = cameraFar + cameraNear;
	float cameraFarMinusNear = cameraFar - cameraNear;
	float cameraCoef = 2.0 * cameraNear;

	// user variables

	const int samples = 8;					// ao sample count
	const float radius = 5.0;				// ao radius

	const bool useNoise = true;			// use noise instead of pattern for sample dithering
	const float noiseAmount = 0.0003;		// dithering amount

	const float diffArea = 0.4;				// self-shadowing reduction
	const float gDisplace = 0.4;			// gauss bell center

	in vec2 uv;
	out vec4 outColor;


	float luma(vec3 color) {
		return dot(color, vec3(0.299, 0.587, 0.114));
	}

	// RGBA depth

	/*float unpackDepth(const in vec4 rgba_depth) {
		const vec4 bit_shift = vec4(1.0 / (256.0 * 256.0 * 256.0), 1.0 / (256.0 * 256.0), 1.0 / 256.0, 1.0);
		float depth = dot(rgba_depth, bit_shift);

		return depth;
	}*/

	// generating noise / pattern texture for dithering

	vec2 rand(const vec2 coord) {
		vec2 noise;

		if (useNoise) {
			float nx = dot(coord, vec2(12.9898, 78.233));
			float ny = dot(coord, vec2(12.9898, 78.233) * 2.0);

			noise = clamp(fract(43758.5453 * sin(vec2(nx, ny))), 0.0, 1.0);
		}
		else {
			float ff = fract(1.0 - coord.s * (width / 2.0));
			float gg = fract(coord.t * (height / 2.0));

			noise = vec2(0.25, 0.75) * vec2(ff) + vec2(0.75, 0.25) * gg;
		}

		return (noise * 2.0  - 1.0) * noiseAmount;
	}

	float readDepth(const in vec2 coord) {
		// return (2.0 * cameraNear) / (cameraFar + cameraNear - unpackDepth(texture(depthMap, coord)) * (cameraFar - cameraNear));
		//return cameraCoef / (cameraFarPlusNear - unpackDepth(texture(depthMap, coord)) * cameraFarMinusNear);
		return cameraCoef / (cameraFarPlusNear - texture(depthMap, coord).x * cameraFarMinusNear);
	}

	float compareDepths(const in float depth1, const in float depth2, inout int far) {
		float garea = 2.0;                       // gauss bell width
		float diff = (depth1 - depth2) * 100.0;  // depth difference (0-100)

		// reduce left bell width to avoid self-shadowing

		if (diff < gDisplace) {
			garea = diffArea;
		}
		else {
			far = 1;
		}

		float dd = diff - gDisplace;
		float gauss = pow(EULER, -2.0 * dd * dd / (garea * garea));

		return gauss;
	}

	float calcAO(float depth, float dw, float dh) {
		float dd = radius - depth * radius;
		vec2 vv = vec2(dw, dh);

		vec2 coord1 = uv + dd * vv;
		vec2 coord2 = uv - dd * vv;

		int far = 0;
		float temp1 = compareDepths(depth, readDepth(coord1), far);

		// DEPTH EXTRAPOLATION

		if (far > 0) {
			float temp2 = compareDepths(readDepth(coord2), depth, far);
			temp1 += (1.0 - temp1) * temp2;
		}

		return temp1;
	}

	void main() {
		vec2 noise = rand(uv);
		float depth = readDepth(uv);

		float tt = clamp(depth, aoClamp, 1.0);

		float w = (1.0 / width)  / tt + (noise.x * (1.0 - noise.x));
		float h = (1.0 / height) / tt + (noise.y * (1.0 - noise.y));

		float ao = 0.0;

		float dz = 1.0 / float(samples);
		float z = 1.0 - dz / 2.0;
		float l = 0.0;

		for (int i = 0; i <= samples; i++) {
			float r = sqrt( 1.0 - z );

			float pw = cos(l) * r;
			float ph = sin(l) * r;
			ao += calcAO(depth, pw * w, ph * h);
			z = z - dz;
			l = l + DL;
		}

		ao /= float(samples);
		ao = 1.0 - ao;

		outColor.rgb = texture(colorMap, uv).rgb * ao; //vec3(color * mix(vec3(ao), vec3(1.0), luma?? * lumInfluence));  // mix(color * ao, white, luminance)
		outColor.a = luma(outColor.rgb);
		outColor = vec4(ao, ao, ao, 0.0);
	}

#endif