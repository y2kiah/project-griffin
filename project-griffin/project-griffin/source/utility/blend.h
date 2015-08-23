#pragma once
#ifndef GRIFFIN_BLEND_H_
#define GRIFFIN_BLEND_H_

namespace griffin {

	inline float lerp(float a, float b, float t)
	{
		return (1.0f - t) * a + t * b;
	}

	// TODO: spline functions

	

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