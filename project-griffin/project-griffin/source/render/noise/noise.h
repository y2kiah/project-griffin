#pragma once
#ifndef GRIFFIN_RENDER_NOISE_H_
#define GRIFFIN_RENDER_NOISE_H_

#include <utility/enum.h>
#include <cinttypes>

namespace griffin {
	namespace render {
		namespace noise {

			MakeEnum(CoherentNoiseType, uint8_t,
					 (CoherentNoiseType_Linear)
					 (CoherentNoiseType_Cosine)
					 (CoherentNoiseType_Cubic)
					 (CoherentNoiseType_Quintic)
					 (CoherentNoiseType_Perlin)
					 (CoherentNoiseType_Simplex)
					 (CoherentNoiseType_Test)
					 (CoherentNoiseType_Cached)
					 ,);
			
			MakeEnum(MultiFractalOperation, uint8_t,
					 (MultiFractalOperation_Add)
					 (MultiFractalOperation_Multiply)
					 (MultiFractalOperation_Add_Abs)
					 (MultiFractalOperation_Multiply_Abs)
					 (MultiFractalOperation_Pow)
					 (MultiFractalOperation_Exp)
					 ,);
			
			
			// Function Declarations

			// TEMP
			//void initGrad4();

			// Integer noise methods (discrete)
			float integerNoise1(int x,
								int seed = 0);
			float integerNoise2(int x, int y,
								int xSeed = 0, int ySeed = 0);
			float integerNoise3(int x, int y, int z,
								int xSeed = 0, int ySeed = 0, int zSeed = 0);
			float integerNoise4(int x, int y, int z, int w,
								int xSeed = 0, int ySeed = 0, int zSeed = 0, int wSeed = 0);

			// Coherent noise methods (continuous, interpolate integer noise)
			//  these can be optimized for generating large buffers by caching integer noise results, then interpolating cached noise
			float linearNoise1(float x,
							   int seed = 0);
			float linearNoise2(float x, float y,
							   int xSeed = 0, int ySeed = 0);
			float linearNoise3(float x, float y, float z,
							   int xSeed = 0, int ySeed = 0, int zSeed = 0);
			float linearNoise4(float x, float y, float z, float w,
							   int xSeed = 0, int ySeed = 0, int zSeed = 0, int wSeed = 0);

			float cosineNoise1(float x,
							   int seed = 0);
			float cosineNoise2(float x, float y,
							   int xSeed = 0, int ySeed = 0);
			float cosineNoise3(float x, float y, float z,
							   int xSeed = 0, int ySeed = 0, int zSeed = 0);
			float cosineNoise4(float x, float y, float z, float w,
							   int xSeed = 0, int ySeed = 0, int zSeed = 0, int wSeed = 0);

			float cubicNoise1(float x,
							  int seed = 0);
			float cubicNoise2(float x, float y,
							  int xSeed = 0, int ySeed = 0);
			float cubicNoise3(float x, float y, float z,
							  int xSeed = 0, int ySeed = 0, int zSeed = 0);
			float cubicNoise4(float x, float y, float z, float w,
							  int xSeed = 0, int ySeed = 0, int zSeed = 0, int wSeed = 0);

			float quinticNoise1(float x,
								int seed = 0);
			float quinticNoise2(float x, float y,
								int xSeed = 0, int ySeed = 0);
			float quinticNoise3(float x, float y, float z,
								int xSeed = 0, int ySeed = 0, int zSeed = 0);
			float quinticNoise4(float x, float y, float z, float w,
								int xSeed = 0, int ySeed = 0, int zSeed = 0, int wSeed = 0);

			// fast gradient noise as implemented by Perlin, "improved" version
			float perlinNoise1(float x);
			float perlinNoise2(float x, float y);
			float perlinNoise3(float x, float y, float z);
			float perlinNoise4(float x, float y, float z, float w);

			float simplexNoise1(float x);
			float simplexNoise2(float x, float y);
			float simplexNoise3(float x, float y, float z);
			float simplexNoise4(float x, float y, float z, float w);

			/*float testNoise1(float x);
			float testNoise2(float x, float y);
			float testNoise3(float x, float y, float z);
			float testNoise4(float x, float y, float z, float w);

			float cachedNoise1(float x);
			float cachedNoise2(float x, float y);
			float cachedNoise3(float x, float y, float z);
			float cachedNoise4(float x, float y, float z, float w);*/

			// Wrapper for coherent noise functions
			float coherentNoise(int numDimensions, float *v,
								noise::CoherentNoiseType noiseType = CoherentNoiseType_Simplex);

			// Fractal methods (coherent noise combined in octaves)
			float fBm(int numDimensions, float *v, int octaves, float lacunarity, float persistence, float amplitudeStart,
					  MultiFractalOperation operation = MultiFractalOperation_Add,
					  CoherentNoiseType noiseType = CoherentNoiseType_Simplex);

			float multiFractal(int numDimensions, float *v, int octaves, float lacunarity, float roughness, float amplitudeStart,
							   MultiFractalOperation operation = MultiFractalOperation_Add,
							   CoherentNoiseType noiseType = CoherentNoiseType_Simplex);

			struct Noise3Deriv {
				float n;
				float dx;
				float dy;
				float dz;
			};

			Noise3Deriv perlinNoise3Deriv(float x, float y, float z);

			float swissTurbulence(float x, float y, float z, int octaves,
								  float lacunarity = 2.0f, float persistence = 0.5f, float warp = 0.15f);

			float jordanTurbulence(float x, float y, float z,
								   int octaves, float lacunarity = 2.0f,
								   float gain1 = 0.8f, float gain = 0.5f,
								   float warp0 = 0.4f, float warp = 0.35f,
								   float damp0 = 1.0f, float damp = 0.8f,
								   float damp_scale = 1.0f);

			// Inline Function Definitions

			inline float integerNoise1(int x,
									   int seed)
			{
				x += seed;
				x = (x << 13) ^ x;
				return (1.0f - ((x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) * 0.000000000931322574615478515625f);
			}

			inline float integerNoise2(int x, int y,
									   int xSeed, int ySeed)
			{
				x += xSeed;
				y += ySeed;
				x += y * 47;
				x = (x << 13) ^ x;
				return (1.0f - ((x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) * 0.000000000931322574615478515625f);
			}

			inline float integerNoise3(int x, int y, int z,
									   int xSeed, int ySeed, int zSeed)
			{
				x += xSeed;
				y += ySeed;
				z += zSeed;
				x += (y * 47) + (z * 59);
				x = (x << 13) ^ x;
				return (1.0f - ((x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) * 0.000000000931322574615478515625f);
			}

			inline float integerNoise4(int x, int y, int z, int w,
									   int xSeed, int ySeed, int zSeed, int wSeed)
			{
				x += xSeed;
				y += ySeed;
				z += zSeed;
				w += wSeed;
				x += (y * 47) + (z * 59) + (w * 131);
				x = (x << 13) ^ x;
				return (1.0f - ((x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) * 0.000000000931322574615478515625f);
			}

		}
	}
}

#endif