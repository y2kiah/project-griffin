#include "source/render/shaders/layout.glsli"

#define FXAA_REDUCE_MIN   (1.0/128.0)

uniform float FXAA_SUBPIX_SHIFT = 1.0/4.0;
uniform float FXAA_SPAN_MAX = 8.0;
uniform float FXAA_REDUCE_MUL = 1.0/8.0;

uniform float viewportWidth = 1600;
uniform float viewportHeight = 900;

vec2 rcpFrame = vec2(1.0/viewportWidth, 1.0/viewportHeight);

#ifdef _VERTEX_
	
	layout(location = VertexLayout_Position) in vec3 vertexPosition;

	const vec2 madd = vec2(0.5, 0.5);

	out vec4 posPos;

	void main()
	{
		gl_Position = vec4(vertexPosition, 1.0);

		posPos.xy = vertexPosition.xy * madd + madd;
		posPos.zw = posPos.xy - (rcpFrame * (0.5 + FXAA_SUBPIX_SHIFT));
	}

#endif

#ifdef _FRAGMENT_

	uniform sampler2D colorMap;		// 0

	in vec4 posPos;

	out vec3 outColor;

/*	vec3 fxaaPixelShader(
			vec4 posPos,   // Output of vertex interpolated across screen.
			sampler2D tex, // Input texture.
			vec2 rcpFrame) // Constant {1.0/frameWidth, 1.0/frameHeight}.
	{
		vec3 rgbNW = textureLod(tex, posPos.zw, 0.0).xyz;
		vec3 rgbNE = textureLodOffset(tex, posPos.zw, 0.0, ivec2(1,0)).xyz;
		vec3 rgbSW = textureLodOffset(tex, posPos.zw, 0.0, ivec2(0,1)).xyz;
		vec3 rgbSE = textureLodOffset(tex, posPos.zw, 0.0, ivec2(1,1)).xyz;
		vec3 rgbM  = textureLod(tex, posPos.xy, 0.0).xyz;

		vec3 luma = vec3(0.299, 0.587, 0.114);
		float lumaNW = dot(rgbNW, luma);
		float lumaNE = dot(rgbNE, luma);
		float lumaSW = dot(rgbSW, luma);
		float lumaSE = dot(rgbSE, luma);
		float lumaM  = dot(rgbM,  luma);

		float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
		float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

		vec2 dir; 
		dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
		dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

		float dirReduce = max(
				(lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),
				FXAA_REDUCE_MIN);

		float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);

		dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),
				max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
				dir * rcpDirMin)) * rcpFrame.xy;

		vec3 rgbA = (1.0/2.0) * (
				textureLod(tex, posPos.xy + dir * (1.0/3.0 - 0.5), 0.0).xyz +
				textureLod(tex, posPos.xy + dir * (2.0/3.0 - 0.5), 0.0).xyz);

		vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (
				textureLod(tex, posPos.xy + dir * (0.0/3.0 - 0.5), 0.0).xyz +
				textureLod(tex, posPos.xy + dir * (3.0/3.0 - 0.5), 0.0).xyz);

		float lumaB = dot(rgbB, luma);

		if ((lumaB < lumaMin) || (lumaB > lumaMax)) {
			return rgbA;
		}

		return rgbB;
	}
*/
	
	#ifndef FXAA_DISCARD
		// 
		// Only valid for PC OpenGL currently.
		// 
		// 1 = Use discard on pixels which don't need AA.
		//     For APIs which enable concurrent TEX+ROP from same surface.
		// 0 = Return unchanged color on pixels which don't need AA.
		// 
		#define FXAA_DISCARD 0
	#endif    

	#ifndef FXAA_LINEAR
		//
		// 0 = Work in non-linear color space.
		//     Use this for standard 32-bit RGBA formats.
		//
		// 1 = Work in RGB=linear, A=non-linear luma.
		//     Use this for sRGB and FP16 formats. 
		//     Works with either FXAA_ALGORITHM = 1 or 0. 
		//
		#define FXAA_LINEAR 0
	#endif

	#ifndef FXAA_QUALITY__EDGE_THRESHOLD
		//
		// The minimum amount of local contrast required to apply algorithm.
		//
		// 1/3 - too little
		// 1/4 - low quality
		// 1/6 - default
		// 1/8 - high quality
		// 1/16 - overkill
		//
		#define FXAA_QUALITY__EDGE_THRESHOLD (1.0/6.0)
	#endif

	#ifndef FXAA_QUALITY__EDGE_THRESHOLD_MIN
		//
		// Trims the algorithm from processing darks.
		//
		// 1/32 - visible limit
		// 1/16 - high quality
		// 1/12 - upper limit (default, the start of visible unfiltered edges)
		//
		#define FXAA_QUALITY__EDGE_THRESHOLD_MIN (1.0/12.0)
	#endif

	#ifndef FXAA_QUALITY__SUBPIX_CAP
		//
		// Insures fine detail is not completely removed.
		// This partly overrides FXAA_SUBPIX_TRIM.
		//
		// 3/4 - default amount of filtering
		// 7/8 - high amount of filtering
		// 1 - no capping of filtering
		//
		#define FXAA_QUALITY__SUBPIX_CAP (3.0/4.0)
	#endif

	#ifndef FXAA_QUALITY__SUBPIX_TRIM
		//
		// Controls removal of sub-pixel aliasing,
		//
		// 1/2 - low removal (sharper but more sub-pixel aliasing)
		// 1/3 - medium removal
		// 1/4 - default removal
		// 1/8 - high removal
		// 0 - complete removal (softer but less sub-pixel aliasing)
		//
		#define FXAA_QUALITY__SUBPIX_TRIM (1.0/4.0)
	#endif

	vec4 fxaaPixelShader(
			// {xy} = center of pixel
			vec2 pos,
			// {xyzw} = not used on FXAA3 Quality
		//	vec4 posPos,
			// {rgb_} = color in linear or perceptual color space
			// {___a} = luma in perceptual color space (not linear)
			sampler2D tex,
			// This must be from a constant/uniform.
			// {x_} = 1.0/screenWidthInPixels
			// {_y} = 1.0/screenHeightInPixels
			vec2 rcpFrame)
	{
		vec4 luma4A = textureGatherOffset(tex, pos.xy, ivec2(-1, -1), 3);
		#if (FXAA_DISCARD == 0)
			vec4 rgbyM = textureLod(tex, pos.xy, 0.0);
		#endif
		vec4 luma4B = textureGather(tex, pos.xy, 3);
		float lumaNE = textureLodOffset(tex, pos.xy, 0.0, ivec2(1, -1)).w;
		float lumaSW = textureLodOffset(tex, pos.xy, 0.0, ivec2(-1, 1)).w;
		float lumaNW = luma4A.w;
		float lumaN  = luma4A.z;
		float lumaW  = luma4A.x;
		float lumaM  = luma4A.y;
		float lumaE  = luma4B.z;
		float lumaS  = luma4B.x;
		float lumaSE = luma4B.y;

		float rangeMin = min(lumaM, min(min(lumaN, lumaW), min(lumaS, lumaE)));
		float rangeMax = max(lumaM, max(max(lumaN, lumaW), max(lumaS, lumaE)));
		float range = rangeMax - rangeMin;

		if (range < max(FXAA_QUALITY__EDGE_THRESHOLD_MIN, rangeMax * FXAA_QUALITY__EDGE_THRESHOLD)) {
			#if (FXAA_DISCARD == 1)
				discard;
			#else
				return rgbyM;
			#endif
		}

		#define FXAA_QUALITY__SUBPIX_TRIM_SCALE  (1.0/(1.0 - FXAA_QUALITY__SUBPIX_TRIM))

		float lumaL = (lumaN + lumaW + lumaE + lumaS) * 0.25;
		float rangeL = abs(lumaL - lumaM);
		float blendL = clamp((rangeL / range) - FXAA_QUALITY__SUBPIX_TRIM, 0.0, 1.0) * FXAA_QUALITY__SUBPIX_TRIM_SCALE; 
		blendL = min(FXAA_QUALITY__SUBPIX_CAP, blendL);

		float edgeVert = 
					abs(lumaNW + (-2.0 * lumaN) + lumaNE) +
			  2.0 * abs(lumaW  + (-2.0 * lumaM) + lumaE ) +
					abs(lumaSW + (-2.0 * lumaS) + lumaSE);
		float edgeHorz = 
					abs(lumaNW + (-2.0 * lumaW) + lumaSW) +
			  2.0 * abs(lumaN  + (-2.0 * lumaM) + lumaS ) +
					abs(lumaNE + (-2.0 * lumaE) + lumaSE);
		bool horzSpan = (edgeHorz >= edgeVert);

		float lengthSign = horzSpan ? -rcpFrame.y : -rcpFrame.x;
		if (!horzSpan) { lumaN = lumaW; }
		if (!horzSpan) { lumaS = lumaE; }
		float gradientN = abs(lumaN - lumaM);
		float gradientS = abs(lumaS - lumaM);
		lumaN = (lumaN + lumaM) * 0.5;
		lumaS = (lumaS + lumaM) * 0.5;

		bool pairN = (gradientN >= gradientS);
		if (!pairN) { lumaN = lumaS; }
		if (!pairN) { gradientN = gradientS; }
		if (!pairN) { lengthSign *= -1.0; }
		vec2 posN;
		posN.x = pos.x + (horzSpan ? 0.0 : lengthSign * 0.5);
		posN.y = pos.y + (horzSpan ? lengthSign * 0.5 : 0.0);

		#define FXAA_SEARCH_STEPS     6
		#define FXAA_SEARCH_THRESHOLD (1.0/4.0)

		gradientN *= FXAA_SEARCH_THRESHOLD;

		vec2 posP = posN;
		vec2 offNP = horzSpan ? vec2(rcpFrame.x, 0.0) : vec2(0.0f, rcpFrame.y); 
		float lumaEndN;
		float lumaEndP;
		bool doneN = false;
		bool doneP = false;
		posN += offNP * (-1.5);
		posP += offNP * ( 1.5);
		for (int i = 0; i < FXAA_SEARCH_STEPS; i++) {
			lumaEndN = textureLod(tex, posN.xy, 0.0).w;
			lumaEndP = textureLod(tex, posP.xy, 0.0).w;
			bool doneN2 = (abs(lumaEndN - lumaN) >= gradientN);
			bool doneP2 = (abs(lumaEndP - lumaN) >= gradientN);
			if (doneN2 && !doneN) { posN += offNP; }
			if (doneP2 && !doneP) { posP -= offNP; }
			if (doneN2 && doneP2) { break; }
			doneN = doneN2;
			doneP = doneP2;
			if (!doneN) { posN -= offNP * 2.0; }
			if (!doneP) { posP += offNP * 2.0; }
		}

		float dstN = horzSpan ? pos.x - posN.x : pos.y - posN.y;
		float dstP = horzSpan ? posP.x - pos.x : posP.y - pos.y;

		bool directionN = (dstN < dstP);
		lumaEndN = directionN ? lumaEndN : lumaEndP;

		if (((lumaM - lumaN) < 0.0) == ((lumaEndN - lumaN) < 0.0)) {
			lengthSign = 0.0;
		}

		float spanLength = (dstP + dstN);
		dstN = directionN ? dstN : dstP;
		float subPixelOffset = 0.5 + (dstN * (-1.0/spanLength));
		subPixelOffset += blendL * (1.0/8.0);
		subPixelOffset *= lengthSign;
		vec3 rgbF = textureLod(tex, vec2(
				pos.x + (horzSpan ? 0.0 : subPixelOffset),
				pos.y + (horzSpan ? subPixelOffset : 0.0)),
				0.0).xyz;

		#if (FXAA_LINEAR == 1)
			lumaL *= lumaL;
		#endif

		float lumaF = dot(rgbF, vec3(0.299, 0.587, 0.114)) + (1.0/(65536.0*256.0));
		float lumaB = mix(lumaF, lumaL, blendL);
		float scale = min(4.0, lumaB/lumaF);
		rgbF *= scale;
		return vec4(rgbF, lumaM);
	}
	
	void main()
	{
		//const float gamma = 1.8;
		//outColor = pow(fxaaPixelShader(posPos.xy, colorMap, rcpFrame).rgb, vec3(1.0 / gamma));
		//outColor = sqrt(fxaaPixelShader(posPos.xy, colorMap, rcpFrame).rgb); // gamma correction, close to pow(C, 1.0/2.2)
		outColor = fxaaPixelShader(posPos.xy, colorMap, rcpFrame).rgb;
	}

#endif