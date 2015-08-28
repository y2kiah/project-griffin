#pragma once
#ifndef GRIFFIN_BLEND_H_
#define GRIFFIN_BLEND_H_

namespace griffin {

	inline float lerp(float a, float b, float t)
	{
		return (1.0f - t) * a + t * b;
	}

	inline float hermite(float t)
	{
		// 3t^2 - 2t^3
		return t * t * (3.0f - 2.0f * t);
	}

	inline float quintic(float t)
	{
		float t3 = t * t * t;
		return t3 * ((6.0f * t * t) - (15.0f * t) + 10.0f);
	}

	/**
	* @see http://developer.amd.com/wordpress/media/2012/10/Andersson-TerrainRendering(Siggraph07).pdf
	*/
	inline float overlayBlend(float base, float value, float opacity)
	{
		float a = (base < 0.5f) ? (2.0f * base * value) : (1.0f - 2.0f * (1.0f - base) * (1.0f - value));
		return lerp(base, a, opacity);
	}

}

#endif