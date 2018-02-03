#include "noise.h"
#include <cmath>
#include <cassert>
#include <climits>

#define NOISE_MAX_OCTAVES	20

#define PERLIN1_MULT		0.188f	// used to rescale noise output into range of classic Perlin noise
#define PERLIN2_MULT		0.507f
#define PERLIN3_MULT		0.936f	// 3d multiplier not used for performance benefit
#define PERLIN4_MULT		0.87f

#define PIf					3.14159265358979323846f		// PI
#define INV_LN_HALFf		-1.44269504088896340736f	// 1 / ln(.5)


using namespace griffin::render;
using std::round;
using std::floor;


// Perlin noise precomputed permutations, also used in simplex noise
const unsigned char p[512] = {
	151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,   225, 140, 36,  103, 30,
	69,  142, 8,   99,  37,  240, 21,  10,  23,  190, 6,   148, 247, 120, 234, 75,  0,   26,  197, 62,
	94,  252, 219, 203, 117, 35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136,
	171, 168, 68,  175, 74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158, 231, 83,  111, 229, 122,
	60,  211, 133, 230, 220, 105, 92,  41,  55,  46,  245, 40,  244, 102, 143, 54,  65,  25,  63,  161,
	1,   216, 80,  73,  209, 76,  132, 187, 208, 89,  18,  169, 200, 196, 135, 130, 116, 188, 159, 86,
	164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250, 124, 123, 5,   202, 38,  147, 118, 126,
	255, 82,  85,  212, 207, 206, 59,  227, 47,  16,  58,  17,  182, 189, 28,  42,  223, 183, 170, 213,
	119, 248, 152, 2,   44,  154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,   129, 22,  39,  253,
	19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,  228, 251, 34,  242, 193,
	238, 210, 144, 12,  191, 179, 162, 241, 81,  51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,
	181, 199, 106, 157, 184, 84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,
	222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156, 180,

	151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,   225, 140, 36,  103, 30,
	69,  142, 8,   99,  37,  240, 21,  10,  23,  190, 6,   148, 247, 120, 234, 75,  0,   26,  197, 62,
	94,  252, 219, 203, 117, 35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136,
	171, 168, 68,  175, 74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158, 231, 83,  111, 229, 122,
	60,  211, 133, 230, 220, 105, 92,  41,  55,  46,  245, 40,  244, 102, 143, 54,  65,  25,  63,  161,
	1,   216, 80,  73,  209, 76,  132, 187, 208, 89,  18,  169, 200, 196, 135, 130, 116, 188, 159, 86,
	164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250, 124, 123, 5,   202, 38,  147, 118, 126,
	255, 82,  85,  212, 207, 206, 59,  227, 47,  16,  58,  17,  182, 189, 28,  42,  223, 183, 170, 213,
	119, 248, 152, 2,   44,  154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,   129, 22,  39,  253,
	19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,  228, 251, 34,  242, 193,
	238, 210, 144, 12,  191, 179, 162, 241, 81,  51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,
	181, 199, 106, 157, 184, 84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,
	222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156, 180
};

// 4D Simplex noise lookup table
// lookup table to traverse the simplex around a given point in 4D
const unsigned char simplex[64][4] = {
	{ 0, 1, 2, 3 }, { 0, 1, 3, 2 }, { 0, 0, 0, 0 }, { 0, 2, 3, 1 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 1, 2, 3, 0 },
	{ 0, 2, 1, 3 }, { 0, 0, 0, 0 }, { 0, 3, 1, 2 }, { 0, 3, 2, 1 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 1, 3, 2, 0 },
	{ 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
	{ 1, 2, 0, 3 }, { 0, 0, 0, 0 }, { 1, 3, 0, 2 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 2, 3, 0, 1 }, { 2, 3, 1, 0 },
	{ 1, 0, 2, 3 }, { 1, 0, 3, 2 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 2, 0, 3, 1 }, { 0, 0, 0, 0 }, { 2, 1, 3, 0 },
	{ 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
	{ 2, 0, 1, 3 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 3, 0, 1, 2 }, { 3, 0, 2, 1 }, { 0, 0, 0, 0 }, { 3, 1, 2, 0 },
	{ 2, 1, 0, 3 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 3, 1, 0, 2 }, { 0, 0, 0, 0 }, { 3, 2, 0, 1 }, { 3, 2, 1, 0 }
};

// TEMP
/*
void noise::initGrad4()
{
	float kkf[256];
	for (int i = 0; i < 256; ++i) kkf[i] = -1.0f + 2.0f * ((float)i / 255.0f);

	for (int i = 0; i < 256; ++i) {
		grad4[i] = kkf[p[i]] * 0.7f;
	}

	for (int y = 0; y < 256; ++y) {
		for (int x = 0; x < 256; ++x) {
			n[y * 256 + x] = integerNoise2(x, y);
		}
	}
}
*/
// end TEMP


// Helper functions

static inline float clamp(float a, float b, float x)
{
	return (x < a ? a : (x > b ? b : x));
}

static inline float bias(float a, float b)
{
	return powf(a, logf(b) * INV_LN_HALFf);
}

static inline float gamma(float a, float g)
{
	return powf(a, 1.0f / g);
}

static inline float expose(float l, float k)
{
	return (1.0f - expf(-l * k));
}


// Interpolation functions

static inline float lerp(float a, float b, float t)
{
	return a + t * (b - a);
}

// Cubic S-curve = 3t^2 - 2t^3 : 2nd derivative is discontinuous at t=0 and t=1 causing visual artifacts at boundaries
static inline float sCurve(float t)
{
	return t * t * (3.0f - 2.0f * t);
}

// Cubic curve 1st derivative = 6t - 6t^2
static inline float sCurveDeriv(float t)
{
	return 6.0f * t * (1.0f - t);
}

// Quintic curve = 6t^5 - 15t^4 + 10t^3
static inline float qCurve(float t)
{
	return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

// Quintic curve 1st derivative = 30t^4 - 60t^3 + 30t^2
static inline float qCurveDeriv(float t)
{
	return t * t * (t * (t * 30.0f - 60.0f) + 30.0f);
}

// Cosine curve
static inline float cosCurve(float t)
{
	return (1.0f - cosf(t * PIf)) * 0.5f;
}

static float step(float a, float x)
{
	return static_cast<float>(x >= a);
}

static float boxStep(float a, float b, float x)
{
	assert(b != a);
	return clamp(0.0f, 1.0f, (x - a) / (b - a));
}

static float pulse(float a, float b, float x)
{
	return static_cast<float>((x >= a) - (x >= b));
}

// Noise Functions

// Perlin noise gradient methods, also used in simplex noise

inline float grad(int hash, float x)
{
	hash &= 15;
	float grad = 1.0f + (hash & 7);	// gradient value 1.0, 2.0, ..., 8.0
	if (hash & 8) return -grad * x;	// and a random sign for the gradient
	else return grad * x;			// multiply the gradient with the distance
}

inline float grad(int hash, float x, float y)
{
	hash &= 7;						// convert low 3 bits of hash code
	float s = (hash < 4) ? x : y;	// into 8 simple gradient directions
	float t = (hash < 4) ? y : x;	// and compute the dot product with (x, y)
	return ((hash & 1) ? -s : s) + ((hash & 2) ? -2.0f * t : 2.0f * t);
}

inline float grad(int hash, float x, float y, float z)
{
	hash &= 15;						// convert low 4 bits of hash code into 12 simple
	float s = (hash < 8) ? x : y;	// gradient directions and compute dot product
	float t = (hash < 4) ? y : ((hash == 12 || hash == 14) ? x : z);	// fix repeats at hash = 12 to 15
	return ((hash & 1) ? -s : s) + ((hash & 2) ? -t : t);
}

inline float grad(int hash, float x, float y, float z, float w)
{
	hash &= 31;						// Convert low 5 bits of hash code into 32 simple
	float s = (hash < 24) ? x : y;	// gradient directions, and compute dot product.
	float t = (hash < 16) ? y : z;
	float r = (hash < 8) ? z : w;
	return ((hash & 1) ? -s : s) + ((hash & 2) ? -t : t) + ((hash & 4) ? -r : r);
}

// Linear interpolated noise is the most basic type of coherent noise, good for fast
// interpolation of noise values when high visual quality is not necessary.
float noise::linearNoise1(float x,
						  int seed)
{
	int ix = static_cast<int>(floor(x));
	return lerp(integerNoise1(ix,     seed),
				integerNoise1(ix + 1, seed), x - ix);
}

float noise::linearNoise2(float x, float y,
						  int xSeed, int ySeed)
{
	int ix = static_cast<int>(floor(x));
	int iy = static_cast<int>(floor(y));
	int ix1 = ix + 1;
	int iy1 = iy + 1;

	x -= ix;
	y -= iy;

	return lerp(lerp(integerNoise2(ix,  iy,  xSeed, ySeed),
					 integerNoise2(ix1, iy,  xSeed, ySeed), x),
				lerp(integerNoise2(ix,  iy1, xSeed, ySeed),
					 integerNoise2(ix1, iy1, xSeed, ySeed), x), y);
}

float noise::linearNoise3(float x, float y, float z,
						  int xSeed, int ySeed, int zSeed)
{
	int ix = static_cast<int>(floor(x));
	int iy = static_cast<int>(floor(y));
	int iz = static_cast<int>(floor(z));
	int ix1 = ix + 1;
	int iy1 = iy + 1;
	int iz1 = iz + 1;

	x -= ix;
	y -= iy;
	z -= iz;

	return lerp(lerp(lerp(integerNoise3(ix,  iy,  iz,  xSeed, ySeed, zSeed),
						  integerNoise3(ix1, iy,  iz,  xSeed, ySeed, zSeed), x),
					 lerp(integerNoise3(ix,  iy1, iz,  xSeed, ySeed, zSeed),
						  integerNoise3(ix1, iy1, iz,  xSeed, ySeed, zSeed), x), y),
				lerp(lerp(integerNoise3(ix,  iy,  iz1, xSeed, ySeed, zSeed),
						  integerNoise3(ix1, iy,  iz1, xSeed, ySeed, zSeed), x),
					 lerp(integerNoise3(ix,  iy1, iz1, xSeed, ySeed, zSeed),
						  integerNoise3(ix1, iy1, iz1, xSeed, ySeed, zSeed), x), y), z);
}

float noise::linearNoise4(float x, float y, float z, float w,
						  int xSeed, int ySeed, int zSeed, int wSeed)
{
	int ix = static_cast<int>(floor(x));
	int iy = static_cast<int>(floor(y));
	int iz = static_cast<int>(floor(z));
	int iw = static_cast<int>(floor(w));
	int ix1 = ix + 1;
	int iy1 = iy + 1;
	int iz1 = iz + 1;
	int iw1 = iw + 1;

	x -= ix;
	y -= iy;
	z -= iz;
	w -= iw;

	return lerp(lerp(lerp(lerp(integerNoise4(ix,  iy,  iz,  iw,  xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy,  iz,  iw,  xSeed, ySeed, zSeed, wSeed), x),
						  lerp(integerNoise4(ix,  iy1, iz,  iw,  xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy1, iz,  iw,  xSeed, ySeed, zSeed, wSeed), x), y),
					 lerp(lerp(integerNoise4(ix,  iy,  iz1, iw,  xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy,  iz1, iw,  xSeed, ySeed, zSeed, wSeed), x),
						  lerp(integerNoise4(ix,  iy1, iz1, iw,  xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy1, iz1, iw,  xSeed, ySeed, zSeed, wSeed), x), y), z),
				lerp(lerp(lerp(integerNoise4(ix,  iy,  iz,  iw1, xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy,  iz,  iw1, xSeed, ySeed, zSeed, wSeed), x),
						  lerp(integerNoise4(ix,  iy1, iz,  iw1, xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy1, iz,  iw1, xSeed, ySeed, zSeed, wSeed), x), y),
					 lerp(lerp(integerNoise4(ix,  iy,  iz1, iw1, xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy,  iz1, iw1, xSeed, ySeed, zSeed, wSeed), x),
						  lerp(integerNoise4(ix,  iy1, iz1, iw1, xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy1, iz1, iw1, xSeed, ySeed, zSeed, wSeed), x), y), z), w);
}

//----------------------------------------------------------------------------------------
// Cosine noise uses the a sine curve to interpolate between integer values.
//----------------------------------------------------------------------------------------
float noise::cosineNoise1(float x,
						  int seed)
{
	int ix = static_cast<int>(floor(x));
	return lerp(integerNoise1(ix, seed),
				integerNoise1(ix + 1, seed), cosCurve(x - ix));
}

float noise::cosineNoise2(float x, float y,
						  int xSeed, int ySeed)
{
	int ix = static_cast<int>(floor(x));
	int iy = static_cast<int>(floor(y));
	int ix1 = ix + 1;
	int iy1 = iy + 1;

	x = cosCurve(x - ix);
	y = cosCurve(y - iy);

	return lerp(lerp(integerNoise2(ix,  iy,  xSeed, ySeed),
					 integerNoise2(ix1, iy,  xSeed, ySeed), x),
				lerp(integerNoise2(ix,  iy1, xSeed, ySeed),
					 integerNoise2(ix1, iy1, xSeed, ySeed), x), y);
}

float noise::cosineNoise3(float x, float y, float z,
						  int xSeed, int ySeed, int zSeed)
{
	int ix = static_cast<int>(floor(x));
	int iy = static_cast<int>(floor(y));
	int iz = static_cast<int>(floor(z));
	int ix1 = ix + 1;
	int iy1 = iy + 1;
	int iz1 = iz + 1;

	x = cosCurve(x - ix);
	y = cosCurve(y - iy);
	z = cosCurve(z - iz);

	return lerp(lerp(lerp(integerNoise3(ix,  iy,  iz,  xSeed, ySeed, zSeed),
						  integerNoise3(ix1, iy,  iz,  xSeed, ySeed, zSeed), x),
					 lerp(integerNoise3(ix,  iy1, iz,  xSeed, ySeed, zSeed),
						  integerNoise3(ix1, iy1, iz,  xSeed, ySeed, zSeed), x), y),
				lerp(lerp(integerNoise3(ix,  iy,  iz1, xSeed, ySeed, zSeed),
						  integerNoise3(ix1, iy,  iz1, xSeed, ySeed, zSeed), x),
					 lerp(integerNoise3(ix,  iy1, iz1, xSeed, ySeed, zSeed),
						  integerNoise3(ix1, iy1, iz1, xSeed, ySeed, zSeed), x), y), z);
}

float noise::cosineNoise4(float x, float y, float z, float w,
						  int xSeed, int ySeed, int zSeed, int wSeed)
{
	int ix = static_cast<int>(floor(x));
	int iy = static_cast<int>(floor(y));
	int iz = static_cast<int>(floor(z));
	int iw = static_cast<int>(floor(w));
	int ix1 = ix + 1;
	int iy1 = iy + 1;
	int iz1 = iz + 1;
	int iw1 = iw + 1;

	x = cosCurve(x - ix);
	y = cosCurve(y - iy);
	z = cosCurve(z - iz);
	w = cosCurve(w - iw);

	return lerp(lerp(lerp(lerp(integerNoise4(ix,  iy,  iz,  iw,  xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy,  iz,  iw,  xSeed, ySeed, zSeed, wSeed), x),
						  lerp(integerNoise4(ix,  iy1, iz,  iw,  xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy1, iz,  iw,  xSeed, ySeed, zSeed, wSeed), x), y),
					 lerp(lerp(integerNoise4(ix,  iy,  iz1, iw,  xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy,  iz1, iw,  xSeed, ySeed, zSeed, wSeed), x),
						  lerp(integerNoise4(ix,  iy1, iz1, iw,  xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy1, iz1, iw,  xSeed, ySeed, zSeed, wSeed), x), y), z),
				lerp(lerp(lerp(integerNoise4(ix,  iy,  iz,  iw1, xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy,  iz,  iw1, xSeed, ySeed, zSeed, wSeed), x),
						  lerp(integerNoise4(ix,  iy1, iz,  iw1, xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy1, iz,  iw1, xSeed, ySeed, zSeed, wSeed), x), y),
					 lerp(lerp(integerNoise4(ix,  iy,  iz1, iw1, xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy,  iz1, iw1, xSeed, ySeed, zSeed, wSeed), x),
						  lerp(integerNoise4(ix,  iy1, iz1, iw1, xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy1, iz1, iw1, xSeed, ySeed, zSeed, wSeed), x), y), z), w);
}

//----------------------------------------------------------------------------------------
// Cubic noise interpolates with an s-curve, 3t^2 - 2t^3, and improves upon the visual
// quality of linear interpolated noise. The hermite curve has C1 continuity at integer
// boundaries.
//----------------------------------------------------------------------------------------
float noise::cubicNoise1(float x,
						 int seed)
{
	int ix = static_cast<int>(floor(x));
	return lerp(integerNoise1(ix, seed),
				integerNoise1(ix + 1, seed), sCurve(x - ix));
}

float noise::cubicNoise2(float x, float y,
						 int xSeed, int ySeed)
{
	int ix = static_cast<int>(floor(x));
	int iy = static_cast<int>(floor(y));
	int ix1 = ix + 1;
	int iy1 = iy + 1;

	x = sCurve(x - ix);
	y = sCurve(y - iy);

	return lerp(lerp(integerNoise2(ix,  iy,  xSeed, ySeed),
					 integerNoise2(ix1, iy,  xSeed, ySeed), x),
				lerp(integerNoise2(ix,  iy1, xSeed, ySeed),
					 integerNoise2(ix1, iy1, xSeed, ySeed), x), y);
}

float noise::cubicNoise3(float x, float y, float z,
						 int xSeed, int ySeed, int zSeed)
{
	int ix = static_cast<int>(floor(x));
	int iy = static_cast<int>(floor(y));
	int iz = static_cast<int>(floor(z));
	int ix1 = ix + 1;
	int iy1 = iy + 1;
	int iz1 = iz + 1;

	x = sCurve(x - ix);
	y = sCurve(y - iy);
	z = sCurve(z - iz);

	return lerp(lerp(lerp(integerNoise3(ix,  iy,  iz,  xSeed, ySeed, zSeed),
						  integerNoise3(ix1, iy,  iz,  xSeed, ySeed, zSeed), x),
					 lerp(integerNoise3(ix,  iy1, iz,  xSeed, ySeed, zSeed),
						  integerNoise3(ix1, iy1, iz,  xSeed, ySeed, zSeed), x), y),
				lerp(lerp(integerNoise3(ix,  iy,  iz1, xSeed, ySeed, zSeed),
						  integerNoise3(ix1, iy,  iz1, xSeed, ySeed, zSeed), x),
					 lerp(integerNoise3(ix,  iy1, iz1, xSeed, ySeed, zSeed),
						  integerNoise3(ix1, iy1, iz1, xSeed, ySeed, zSeed), x), y), z);
}

float noise::cubicNoise4(float x, float y, float z, float w,
						 int xSeed, int ySeed, int zSeed, int wSeed)
{
	int ix = static_cast<int>(floor(x));
	int iy = static_cast<int>(floor(y));
	int iz = static_cast<int>(floor(z));
	int iw = static_cast<int>(floor(w));
	int ix1 = ix + 1;
	int iy1 = iy + 1;
	int iz1 = iz + 1;
	int iw1 = iw + 1;

	x = sCurve(x - ix);
	y = sCurve(y - iy);
	z = sCurve(z - iz);
	w = sCurve(w - iw);

	return lerp(lerp(lerp(lerp(integerNoise4(ix,  iy,  iz,  iw,  xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy,  iz,  iw,  xSeed, ySeed, zSeed, wSeed), x),
						  lerp(integerNoise4(ix,  iy1, iz,  iw,  xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy1, iz,  iw,  xSeed, ySeed, zSeed, wSeed), x), y),
					 lerp(lerp(integerNoise4(ix,  iy,  iz1, iw,  xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy,  iz1, iw,  xSeed, ySeed, zSeed, wSeed), x),
						  lerp(integerNoise4(ix,  iy1, iz1, iw,  xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy1, iz1, iw,  xSeed, ySeed, zSeed, wSeed), x), y), z),
				lerp(lerp(lerp(integerNoise4(ix,  iy,  iz,  iw1, xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy,  iz,  iw1, xSeed, ySeed, zSeed, wSeed), x),
						  lerp(integerNoise4(ix,  iy1, iz,  iw1, xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy1, iz,  iw1, xSeed, ySeed, zSeed, wSeed), x), y),
					 lerp(lerp(integerNoise4(ix,  iy,  iz1, iw1, xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy,  iz1, iw1, xSeed, ySeed, zSeed, wSeed), x),
						  lerp(integerNoise4(ix,  iy1, iz1, iw1, xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy1, iz1, iw1, xSeed, ySeed, zSeed, wSeed), x), y), z), w);
}

//----------------------------------------------------------------------------------------
// Quintic noise interpolates with the function 6t^5 - 15t^4 + 10t^3, providing C2
// continuity at integer boundaries. This eliminates visual artifacts in the normal map of
// a height field generated by the noise.
//----------------------------------------------------------------------------------------
float noise::quinticNoise1(float x,
						   int seed)
{
	int ix = static_cast<int>(floor(x));
	return lerp(integerNoise1(ix, seed),
				integerNoise1(ix + 1, seed), qCurve(x - ix));
}

float noise::quinticNoise2(float x, float y,
						   int xSeed, int ySeed)
{
	int ix = static_cast<int>(floor(x));
	int iy = static_cast<int>(floor(y));
	int ix1 = ix + 1;
	int iy1 = iy + 1;

	x = qCurve(x - ix);
	y = qCurve(y - iy);

	return lerp(lerp(integerNoise2(ix,  iy,  xSeed, ySeed),
					 integerNoise2(ix1, iy,  xSeed, ySeed), x),
				lerp(integerNoise2(ix,  iy1, xSeed, ySeed),
					 integerNoise2(ix1, iy1, xSeed, ySeed), x), y);
}

float noise::quinticNoise3(float x, float y, float z,
						   int xSeed, int ySeed, int zSeed)
{
	int ix = static_cast<int>(floor(x));
	int iy = static_cast<int>(floor(y));
	int iz = static_cast<int>(floor(z));
	int ix1 = ix + 1;
	int iy1 = iy + 1;
	int iz1 = iz + 1;

	x = qCurve(x - ix);
	y = qCurve(y - iy);
	z = qCurve(z - iz);

	return lerp(lerp(lerp(integerNoise3(ix,  iy,  iz,  xSeed, ySeed, zSeed),
						  integerNoise3(ix1, iy,  iz,  xSeed, ySeed, zSeed), x),
					 lerp(integerNoise3(ix,  iy1, iz,  xSeed, ySeed, zSeed),
						  integerNoise3(ix1, iy1, iz,  xSeed, ySeed, zSeed), x), y),
				lerp(lerp(integerNoise3(ix,  iy,  iz1, xSeed, ySeed, zSeed),
						  integerNoise3(ix1, iy,  iz1, xSeed, ySeed, zSeed), x),
					 lerp(integerNoise3(ix,  iy1, iz1, xSeed, ySeed, zSeed),
						  integerNoise3(ix1, iy1, iz1, xSeed, ySeed, zSeed), x), y), z);
}

float noise::quinticNoise4(float x, float y, float z, float w,
						   int xSeed, int ySeed, int zSeed, int wSeed)
{
	int ix = static_cast<int>(floor(x));
	int iy = static_cast<int>(floor(y));
	int iz = static_cast<int>(floor(z));
	int iw = static_cast<int>(floor(w));
	int ix1 = ix + 1;
	int iy1 = iy + 1;
	int iz1 = iz + 1;
	int iw1 = iw + 1;

	x = qCurve(x - ix);
	y = qCurve(y - iy);
	z = qCurve(z - iz);
	w = qCurve(w - iw);

	return lerp(lerp(lerp(lerp(integerNoise4(ix,  iy,  iz,  iw,  xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy,  iz,  iw,  xSeed, ySeed, zSeed, wSeed), x),
						  lerp(integerNoise4(ix,  iy1, iz,  iw,  xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy1, iz,  iw,  xSeed, ySeed, zSeed, wSeed), x), y),
					 lerp(lerp(integerNoise4(ix,  iy,  iz1, iw,  xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy,  iz1, iw,  xSeed, ySeed, zSeed, wSeed), x),
						  lerp(integerNoise4(ix,  iy1, iz1, iw,  xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy1, iz1, iw,  xSeed, ySeed, zSeed, wSeed), x), y), z),
				lerp(lerp(lerp(integerNoise4(ix,  iy,  iz,  iw1, xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy,  iz,  iw1, xSeed, ySeed, zSeed, wSeed), x),
						  lerp(integerNoise4(ix,  iy1, iz,  iw1, xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy1, iz,  iw1, xSeed, ySeed, zSeed, wSeed), x), y),
					 lerp(lerp(integerNoise4(ix,  iy,  iz1, iw1, xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy,  iz1, iw1, xSeed, ySeed, zSeed, wSeed), x),
						  lerp(integerNoise4(ix,  iy1, iz1, iw1, xSeed, ySeed, zSeed, wSeed),
							   integerNoise4(ix1, iy1, iz1, iw1, xSeed, ySeed, zSeed, wSeed), x), y), z), w);
}

//----------------------------------------------------------------------------------------
// The following functions output "improved" Perlin noise, and do not require the use of
// the integer noise basis function, and are not affected by the seed values. The
// multipliers for return values are present to scale the results into the range of
// classic Perlin noise, and prevent oversaturation of fBm results.
//----------------------------------------------------------------------------------------
float noise::perlinNoise1(float x)
{
	int ix = static_cast<int>(floor(x));

	x -= ix;
	ix &= 255;

	return lerp(grad(p[ix], x), grad(p[ix + 1], x - 1), qCurve(x)) * PERLIN1_MULT;
}

float noise::perlinNoise2(float x, float y)
{
	int ix = static_cast<int>(floor(x));
	int iy = static_cast<int>(floor(y));

	x -= ix;
	y -= iy;
	float x1 = x - 1.0f;
	float y1 = y - 1.0f;

	ix &= 255;
	iy &= 255;

	float s = qCurve(x);
	float t = qCurve(y);

	int a = p[ix]     + iy;
	int b = p[ix + 1] + iy;

	return lerp(lerp(grad(p[a],     x,  y),
					 grad(p[b],     x1, y),  s),
				lerp(grad(p[a + 1], x,  y1),
					 grad(p[b + 1], x1, y1), s), t) * PERLIN2_MULT;
}

float noise::perlinNoise3(float x, float y, float z)
{
	// Find unit grid cell containing point
	int ix = static_cast<int>(floor(x));
	int iy = static_cast<int>(floor(y));
	int iz = static_cast<int>(floor(z));

	// Find relative x y z of point in cube
	x -= ix;
	y -= iy;
	z -= iz;
	float x1 = x - 1.0f;
	float y1 = y - 1.0f;
	float z1 = z - 1.0f;

	// Wrap integer cells at a 255 period
	ix &= 255;
	iy &= 255;
	iz &= 255;

	float s = qCurve(x);
	float t = qCurve(y);
	float u = qCurve(z);

	// Hash coordinates of the 8 cube corners
	int a  = p[ix]     + iy;
	int aa = p[a]      + iz;
	int ab = p[a + 1]  + iz;
	int b  = p[ix + 1] + iy;
	int ba = p[b]      + iz;
	int bb = p[b + 1]  + iz;

	// Add blended results from 8 cube corners
	return lerp(lerp(lerp(grad(p[aa],     x,  y,  z),
						  grad(p[ba],     x1, y,  z),  s),
					 lerp(grad(p[ab],     x,  y1, z),
						  grad(p[bb],     x1, y1, z),  s), t),
				lerp(lerp(grad(p[aa + 1], x,  y,  z1),
						  grad(p[ba + 1], x1, y,  z1), s),
					 lerp(grad(p[ab + 1], x,  y1, z1),
						  grad(p[bb + 1], x1, y1, z1), s), t), u); // * PERLIN3_MULT;
}

float noise::perlinNoise4(float x, float y, float z, float w)
{
	int ix = static_cast<int>(floor(x));
	int iy = static_cast<int>(floor(y));
	int iz = static_cast<int>(floor(z));
	int iw = static_cast<int>(floor(w));

	x -= ix;
	y -= iy;
	z -= iz;
	w -= iw;
	float x1 = x - 1.0f;
	float y1 = y - 1.0f;
	float z1 = z - 1.0f;
	float w1 = w - 1.0f;

	ix &= 255;
	iy &= 255;
	iz &= 255;
	iw &= 255;

	float s = qCurve(x);
	float t = qCurve(y);
	float u = qCurve(z);
	float v = qCurve(w);

	// Hash coordinates of the 16 corners
	int a   = p[ix]     + iy;
	int aa  = p[a]      + iz;
	int ab  = p[a + 1]  + iz;
	int b   = p[ix + 1] + iy;
	int ba  = p[b]      + iz;
	int bb  = p[b + 1]  + iz;
	int aaa = p[aa]     + iw;
	int aba = p[ab]     + iw;
	int aab = p[aa + 1] + iw;
	int abb = p[ab + 1] + iw;
	int	baa = p[ba]     + iw;
	int bba = p[bb]     + iw;
	int bab = p[ba + 1] + iw;
	int bbb = p[bb + 1] + iw;

	// Add blended results from 16 corners
	return lerp(lerp(lerp(lerp(grad(p[aaa],     x,  y,  z,  w),
							   grad(p[baa],     x1, y,  z,  w),  s),
						  lerp(grad(p[aba],     x,  y1, z,  w),
							   grad(p[bba],     x1, y1, z,  w),  s), t),
					 lerp(lerp(grad(p[aab],     x,  y,  z1, w),
							   grad(p[bab],     x1, y,  z1, w),  s),
						  lerp(grad(p[abb],     x,  y1, z1, w),
							   grad(p[bbb],     x1, y1, z1, w),  s), t), u),
				lerp(lerp(lerp(grad(p[aaa + 1], x,  y,  z,  w1),
							   grad(p[baa + 1], x1, y,  z,  w1), s),
						  lerp(grad(p[aba + 1], x,  y1, z,  w1),
							   grad(p[bba + 1], x1, y1, z,  w1), s), t),
					 lerp(lerp(grad(p[aab + 1], x,  y,  z1, w1),
							   grad(p[bab + 1], x1, y,  z1, w1), s),
						  lerp(grad(p[abb + 1], x,  y1, z1, w1),
							   grad(p[bbb + 1], x1, y1, z1, w1), s), t), u), v) * PERLIN4_MULT;
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
float noise::simplexNoise1(float x)
{
	int ix = static_cast<int>(floor(x));

	x -= ix;
	float x1 = x - 1.0f;

	ix &= 255;

	float t0 = 1.0f - x*x;
	t0 *= t0;
	float t1 = 1.0f - x1*x1;
	t1 *= t1;

	// the maximum value of this noise is 8*(3/4)^4 = 2.53125
	// so a factor of 0.395 scales it to fit exactly within [-1,1]
	return 0.395f * ((t0 * t0 * grad(p[ix], x)) + (t1 * t1 * grad(p[ix + 1], x1)));
}

float noise::simplexNoise2(float x, float y)
{
	const float F2 = 0.366025403f;	// F2 = 0.5*(sqrt(3.0)-1.0)
	const float G2 = 0.211324865f;	// G2 = (3.0-sqrt(3.0))/6.0

	float s = (x + y) * F2;
	int ix = static_cast<int>(floor(x + s));
	int iy = static_cast<int>(floor(y + s));

	float t = (ix + iy) * G2;
	x -= (ix - t);
	y -= (iy - t);

	int ix1, iy1;
	if (x > y) {
		ix1 = 1; iy1 = 0;
	}
	else {
		ix1 = 0; iy1 = 1;
	}

	float x1 = x - ix1 + G2;
	float y1 = y - iy1 + G2;
	float x2 = x - 1.0f + (2.0f * G2);
	float y2 = y - 1.0f + (2.0f * G2);

	ix &= 255;
	iy &= 255;

	float n = 0.0f;
	t = 0.5f - x*x - y*y;
	if (t >= 0.0f) {
		t *= t;
		n = t * t * grad(p[ix + p[iy]], x, y);
	}

	t = 0.5f - x1*x1 - y1*y1;
	if (t >= 0.0f) {
		t *= t;
		n += t * t * grad(p[ix + ix1 + p[iy + iy1]], x1, y1);
	}

	t = 0.5f - x2*x2 - y2*y2;
	if (t >= 0.0f) {
		t *= t;
		n += t * t * grad(p[ix + 1 + p[iy + 1]], x2, y2);
	}

	return 40.0f * n;
}

float noise::simplexNoise3(float x, float y, float z)
{
	const float F3 = 0.333333333f;
	const float G3 = 0.166666667f;

	float s = (x + y + z) * F3;
	int ix = static_cast<int>(floor(x + s));
	int iy = static_cast<int>(floor(y + s));
	int iz = static_cast<int>(floor(z + s));

	float t = (ix + iy + iz) * G3;
	x -= (ix - t);
	y -= (iy - t);
	z -= (iz - t);

	int ix1, iy1, iz1;
	int ix2, iy2, iz2;

	if (x >= y) {
		if (y >= z) {
			ix1 = 1; iy1 = 0; iz1 = 0; ix2 = 1; iy2 = 1; iz2 = 0;
		}
		else if (x >= z) {
			ix1 = 1; iy1 = 0; iz1 = 0; ix2 = 1; iy2 = 0; iz2 = 1;
		}
		else {
			ix1 = 0; iy1 = 0; iz1 = 1; ix2 = 1; iy2 = 0; iz2 = 1;
		}
	}
	else {
		if (y < z) {
			ix1 = 0; iy1 = 0; iz1 = 1; ix2 = 0; iy2 = 1; iz2 = 1;
		}
		else if (x < z) {
			ix1 = 0; iy1 = 1; iz1 = 0; ix2 = 0; iy2 = 1; iz2 = 1;
		}
		else {
			ix1 = 0; iy1 = 1; iz1 = 0; ix2 = 1; iy2 = 1; iz2 = 0;
		}
	}

	float x1 = x - ix1 + G3;
	float y1 = y - iy1 + G3;
	float z1 = z - iz1 + G3;
	float x2 = x - ix2 + (2.0f * G3);
	float y2 = y - iy2 + (2.0f * G3);
	float z2 = z - iz2 + (2.0f * G3);
	float x3 = x - 1.0f + (3.0f * G3);
	float y3 = y - 1.0f + (3.0f * G3);
	float z3 = z - 1.0f + (3.0f * G3);

	ix &= 255;
	iy &= 255;
	iz &= 255;

	float n = 0.0f;
	t = 0.6f - x*x - y*y - z*z;
	if (t >= 0.0f) {
		t *= t;
		n = t * t * grad(p[ix + p[iy + p[iz]]], x, y, z);
	}

	t = 0.6f - x1*x1 - y1*y1 - z1*z1;
	if (t >= 0.0f) {
		t *= t;
		n += t * t * grad(p[ix + ix1 + p[iy + iy1 + p[iz + iz1]]], x1, y1, z1);
	}

	t = 0.6f - x2*x2 - y2*y2 - z2*z2;
	if (t >= 0.0f) {
		t *= t;
		n += t * t * grad(p[ix + ix2 + p[iy + iy2 + p[iz + iz2]]], x2, y2, z2);
	}

	t = 0.6f - x3*x3 - y3*y3 - z3*z3;
	if (t >= 0.0f) {
		t *= t;
		n += t * t * grad(p[ix + 1 + p[iy + 1 + p[iz + 1]]], x3, y3, z3);
	}

	return 32.0f * n;
}

float noise::simplexNoise4(float x, float y, float z, float w)
{
	// the skewing and unskewing factors are hairy again for the 4D case
	const float F4 = 0.309016994f;	// F4 = (sqrt(5.0)-1.0)/4.0
	const float G4 = 0.138196601f;	// G4 = (5.0-sqrt(5.0))/20.0

	// skew the (x,y,z,w) space to determine which cell of 24 simplices we're in
	float s = (x + y + z + w) * F4;	// factor for 4D skewing
	int ix = static_cast<int>(floor(x + s));
	int iy = static_cast<int>(floor(y + s));
	int iz = static_cast<int>(floor(z + s));
	int iw = static_cast<int>(floor(w + s));

	float t = (ix + iy + iz + iw) * G4;	// factor for 4D unskewing
	x -= (ix - t);  // unskew the cell origin back to (x,y,z,w) space
	y -= (iy - t);	// the x,y,z,w distances from the cell origin
	z -= (iz - t);
	w -= (iw - t);

	// The method below finds the ordering of x,y,z,w and then finds the correct
	// traversal order for the simplex we’re in.
	int c = ((x > y) ? 32 : 0) +
		((x > z) ? 16 : 0) +
		((y > z) ? 8 : 0) +
		((x > w) ? 4 : 0) +
		((y > w) ? 2 : 0) +
		((z > w) ? 1 : 0);

	// simplex[c] is a 4-vector with the numbers 0, 1, 2 and 3 in some order
	// many values of c will never occur, since e.g. x>y>z>w makes x<z, y<w and x<w
	// impossible. Only the 24 indices which have non-zero entries make any sense.
	// We use a thresholding to set the coordinates in turn from the largest magnitude.
	// the number 3 in the "simplex" array is at the position of the largest coordinate.
	int ix1 = (simplex[c][0] >= 3) ? 1 : 0;	// the integer offsets for the second simplex corner
	int iy1 = (simplex[c][1] >= 3) ? 1 : 0;
	int iz1 = (simplex[c][2] >= 3) ? 1 : 0;
	int iw1 = (simplex[c][3] >= 3) ? 1 : 0;
	// the number 2 in the "simplex" array is at the second largest coordinate.
	int ix2 = (simplex[c][0] >= 2) ? 1 : 0; // the integer offsets for the third simplex corner
	int iy2 = (simplex[c][1] >= 2) ? 1 : 0;
	int iz2 = (simplex[c][2] >= 2) ? 1 : 0;
	int iw2 = (simplex[c][3] >= 2) ? 1 : 0;
	// the number 1 in the "simplex" array is at the second smallest coordinate.
	int ix3 = (simplex[c][0] >= 1) ? 1 : 0; // the integer offsets for the fourth simplex corner
	int iy3 = (simplex[c][1] >= 1) ? 1 : 0;
	int iz3 = (simplex[c][2] >= 1) ? 1 : 0;
	int iw3 = (simplex[c][3] >= 1) ? 1 : 0;
	// the fifth corner has all coordinate offsets = 1, so no need to look that up.

	float x1 = x - ix1 + G4;			// offsets for second corner in (x,y,z,w) coords
	float y1 = y - iy1 + G4;
	float z1 = z - iz1 + G4;
	float w1 = w - iw1 + G4;
	float x2 = x - ix2 + (2.0f * G4);	// offsets for third corner in (x,y,z,w) coords
	float y2 = y - iy2 + (2.0f * G4);
	float z2 = z - iz2 + (2.0f * G4);
	float w2 = w - iw2 + (2.0f * G4);
	float x3 = x - ix3 + (3.0f * G4);	// offsets for fourth corner in (x,y,z,w) coords
	float y3 = y - iy3 + (3.0f * G4);
	float z3 = z - iz3 + (3.0f * G4);
	float w3 = w - iw3 + (3.0f * G4);
	float x4 = x - 1.0f + (4.0f * G4);	// offsets for last corner in (x,y,z,w) coords
	float y4 = y - 1.0f + (4.0f * G4);
	float z4 = z - 1.0f + (4.0f * G4);
	float w4 = w - 1.0f + (4.0f * G4);

	// wrap the integer indices at 256, to avoid indexing p[] out of bounds
	ix &= 255;
	iy &= 255;
	iz &= 255;
	iw &= 255;

	// calculate the contribution from the five corners
	float n = 0.0f;
	t = 0.6f - x*x - y*y - z*z - w*w;
	if (t >= 0.0f) {
		t *= t;
		n = t * t * grad(p[ix + p[iy + p[iz + p[iw]]]], x, y, z, w);
	}

	t = 0.6f - x1*x1 - y1*y1 - z1*z1 - w1*w1;
	if (t >= 0.0f) {
		t *= t;
		n += t * t * grad(p[ix + ix1 + p[iy + iy1 + p[iz + iz1 + p[iw + iw1]]]], x1, y1, z1, w1);
	}

	t = 0.6f - x2*x2 - y2*y2 - z2*z2 - w2*w2;
	if (t >= 0.0f) {
		t *= t;
		n += t * t * grad(p[ix + ix2 + p[iy + iy2 + p[iz + iz2 + p[iw + iw2]]]], x2, y2, z2, w2);
	}

	t = 0.6f - x3*x3 - y3*y3 - z3*z3 - w3*w3;
	if (t >= 0.0f) {
		t *= t;
		n += t * t * grad(p[ix + ix3 + p[iy + iy3 + p[iz + iz3 + p[iw + iw3]]]], x3, y3, z3, w3);
	}

	t = 0.6f - x4*x4 - y4*y4 - z4*z4 - w4*w4;
	if (t >= 0.0f) {
		t *= t;
		n += t * t * grad(p[ix + 1 + p[iy + 1 + p[iz + 1 + p[iw + 1]]]], x4, y4, z4, w4);
	}

	return 27.0f * n;
}

float noise::coherentNoise(int numDimensions, float *v,
						   noise::CoherentNoiseType noiseType)
{
	assert(numDimensions >= 1 && numDimensions <= 4);

	#define switchDimensions(func1,func2,func3,func4)		switch (numDimensions) {\
																case 1: return func1(v[0]);\
																case 2: return func2(v[0],v[1]);\
																case 3: return func3(v[0],v[1],v[2]);\
																case 4: return func4(v[0],v[1],v[2],v[3]);}

	switch (noiseType) {
		case CoherentNoiseType_Linear:
			switchDimensions(linearNoise1, linearNoise2, linearNoise3, linearNoise4)

		case CoherentNoiseType_Cosine:
			switchDimensions(cosineNoise1, cosineNoise2, cosineNoise3, cosineNoise4)

		case CoherentNoiseType_Cubic:
			switchDimensions(cubicNoise1, cubicNoise2, cubicNoise3, cubicNoise4)

		case CoherentNoiseType_Quintic:
			switchDimensions(quinticNoise1, quinticNoise2, quinticNoise3, quinticNoise4)

		case CoherentNoiseType_Perlin:
			switchDimensions(perlinNoise1, perlinNoise2, perlinNoise3, perlinNoise4)

		case CoherentNoiseType_Simplex:
			switchDimensions(simplexNoise1, simplexNoise2, simplexNoise3, simplexNoise4)

		/*case CoherentNoiseType_Test:
			switchDimensions(testNoise1, testNoise2, testNoise3, testNoise4)

		case CoherentNoiseType_Cached:
			switchDimensions(cachedNoise1, cachedNoise2, cachedNoise3, cachedNoise4)*/

	}
	return 0.0f;
}

// Fractal methods

float noise::fBm(int numDimensions, float *v, int octaves, float lacunarity, float persistence, float amplitudeStart,
				 noise::MultiFractalOperation operation, noise::CoherentNoiseType noiseType)
{
	assert(octaves >= 1 && octaves <= NOISE_MAX_OCTAVES);
	assert(numDimensions >= 1 && numDimensions <= 4);

	float value = 0.0f;
	if (operation == MultiFractalOperation_Multiply || operation == MultiFractalOperation_Multiply_Abs || operation == MultiFractalOperation_Exp) {
		value = 1.0f;
	}

	float lv[4] = {};
	memcpy_s(&lv, sizeof(lv), v, numDimensions * sizeof(float));

	float amplitude = amplitudeStart;

	for (int o = 0; o < octaves; ++o) {
		float octaveValue = coherentNoise(numDimensions, lv, noiseType);
		// pick the fractal operation to perform at each octave
		switch (operation) {
			case MultiFractalOperation_Add:
				value += octaveValue * amplitude;
				break;
			case MultiFractalOperation_Multiply:
				value *= octaveValue * amplitude;
				break;
			case MultiFractalOperation_Add_Abs:
				value += fabs(octaveValue) * amplitude;
				break;
			case MultiFractalOperation_Multiply_Abs:
				value *= fabs(octaveValue) * amplitude;
				break;
			case MultiFractalOperation_Pow:
				value = powf(value, octaveValue) * amplitude;
				break;
			case MultiFractalOperation_Exp:
				value *= expf(octaveValue) * amplitude;
				break;
		}

		amplitude *= persistence;

		for (int i = 0; i < numDimensions; ++i) {
			lv[i] *= lacunarity;
		}
	}

	return value;
}

float noise::multiFractal(int numDimensions, float *v, int octaves, float lacunarity, float roughness, float amplitudeStart,
						  noise::MultiFractalOperation operation, CoherentNoiseType noiseType)
{
	assert(octaves >= 1 && octaves <= NOISE_MAX_OCTAVES);

	float value = 0.0f;
	if (operation == MultiFractalOperation_Multiply || operation == MultiFractalOperation_Multiply_Abs || operation == MultiFractalOperation_Exp) {
		value = 1.0f;
	}

	float amplitude = amplitudeStart;

	for (int o = 0; o < octaves; ++o) {
		float octaveValue = coherentNoise(numDimensions, v, noiseType);
		switch (operation) { // pick the fractal operation to perform at each octave
			case MultiFractalOperation_Add:
				value += octaveValue * amplitude;
				break;
			case MultiFractalOperation_Multiply:
				value *= octaveValue * amplitude;
				break;
			case MultiFractalOperation_Add_Abs:
				value += fabs(octaveValue) * amplitude;
				break;
			case MultiFractalOperation_Multiply_Abs:
				value *= fabs(octaveValue) * amplitude;
				break;
			case MultiFractalOperation_Pow:
				value = powf(value, octaveValue) * amplitude;
				break;
			case MultiFractalOperation_Exp:
				value *= expf(octaveValue) * amplitude;
				break;
		}

		float fromSeaLevel = (value + 1.0f) * 0.5f;
		amplitude = abs(fromSeaLevel * fromSeaLevel * octaveValue * roughness);
		for (int i = 0; i < numDimensions; ++i) v[i] *= lacunarity;
	}

	return value;
}

// TEMP
/*float noise::testNoise1(float x) { return 0; }
float noise::testNoise3(float x, float y, float z) { return 0; }
float noise::testNoise4(float x, float y, float z, float w) { return 0; }

float noise::testNoise2(float x, float y)
{
	int ix = f2iRoundFast(x - 0.5f);
	int iy = f2iRoundFast(y - 0.5f);

	x -= ix; y -= iy;
	//float x1 = x-1.0f; float y1 = y-1.0f;

	ix &= 255; iy &= 255;

	float s = qCurve(x);
	float t = qCurve(y);

	int a = p[ix] + iy;
	int b = p[ix + 1] + iy;

	return lerp(lerp(grad4[a],// grad(p[a  ],x ,y ),
					 grad4[b], s),//grad(p[b  ],x1,y ), s),
				lerp(grad4[a + 1],//grad(p[a+1],x ,y1),
					 grad4[b + 1], s), t); //grad(p[b+1],x1,y1), s), t) * PERLIN2_MULT;
}

float noise::cachedNoise1(float x) { return 0; }
float noise::cachedNoise3(float x, float y, float z) { return 0; }
float noise::cachedNoise4(float x, float y, float z, float w) { return 0; }

float noise::cachedNoise2(float x, float y)
{
	int ix = f2iRoundFast(x - 0.5f);
	int iy = f2iRoundFast(y - 0.5f);
	int ix1 = ix + 1; int iy1 = iy + 1;

	x = qCurve(x - ix);
	y = qCurve(y - iy);

	return lerp(lerp(n[iy  * 256 + ix],
					 n[iy  * 256 + ix1], x),
				lerp(n[iy1 * 256 + ix],
					 n[iy1 * 256 + ix1], x), y);
}
*/
// end TEMP

/*
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/common.hpp>

using namespace glm;

vec3 perlinNoiseDeriv(vec2 p, float seed)
{
	// Calculate 2D integer coordinates i and fraction f.
	vec2 i = floor(p);
	vec2 f = p - i;

	// Get weights from the coordinate fraction
	vec2 w = f * f * f * (f * (f * 6.0f - 15.0f) + 10.0f); // 6f^5 - 15f^4 + 10f^3
	vec4 w4 = vec4(1, w.x, w.y, w.x * w.y);

	// Get the derivative dw/df
	vec2 dw = f * f * (f * (f * 30.0f - 60.0f) + 30.0f); // 30f^4 - 60f^3 + 30f^2

	// Get the derivative d(w*f)/df
	vec2 dwp = f * f * f * (f * (f * 36.0f - 75.0f) + 40.0f); // 36f^5 - 75f^4 + 40f^3

	i /= 256.0f;

	// Get the four randomly permutated indices from the noise lattice nearest to
	// p and offset these numbers with the seed number.
	vec4 perm = tex2D(samplerPerlinPerm2D, i / 256.0f) + seed;
	int permx = p[i.x] + i.y;
	int permy = p[i.x + 1] + i.y;

	// Permutate the four offseted indices again and get the 2D gradient for each
	// of the four permutated coordinates-seed pairs.
	vec4 g1 = tex2D(samplerPerlinGrad2D, perm.xy) * 2.0f - 1.0f;
	vec4 g2 = tex2D(samplerPerlinGrad2D, perm.zw) * 2.0f - 1.0f;

	// Evaluate the four lattice gradients at p
	float a = dot(g1.xy, f);
	float b = dot(g2.xy, f + vec2(-1.0f,  0.0f));
	float c = dot(g1.zw, f + vec2( 0.0f, -1.0f));
	float d = dot(g2.zw, f + vec2(-1.0f, -1.0f));

	// Bi-linearly blend between the gradients, using w4 as blend factors.
	vec4 grads = vec4(a, b - a, c - a, a - b - c + d);
	float n = dot(grads, w4);

	// Calculate the derivatives dn/dx and dn/dy
	float dx = (g1.x + (g1.z-g1.x)*w.y) + 
				((g2.y-g1.y)*f.y - g2.x +
					((g1.y-g2.y-g1.w+g2.w)*f.y + g2.x + g1.w - g2.z - g2.w)*w.y
				) * dw.x +
				((g2.x-g1.x) + (g1.x-g2.x-g1.z+g2.z)*w.y)*dwp.x;
	float dy = (g1.y + (g2.y-g1.y)*w.x) +
				((g1.z-g1.x)*f.x - g1.w +
					((g1.x-g2.x-g1.z+g2.z)*f.x + g2.x + g1.w - g2.z - g2.w)*w.x
				) * dw.y +
				((g1.w-g1.y) + (g1.y-g2.y-g1.w+g2.w)*w.x)*dwp.y;

	// Return the noise value, roughly normalized in the range [-1, 1]
	// Also return the pseudo dn/dx and dn/dy, scaled by the same factor
	return vec3(n, dx, dy) * 1.5f;
}
*/


noise::Noise3Deriv noise::perlinNoise3Deriv(float x, float y, float z)
{
	Noise3Deriv out{};

	int ix = static_cast<int>(floor(x));
	int iy = static_cast<int>(floor(y));
	int iz = static_cast<int>(floor(z));

	x -= ix;
	y -= iy;
	z -= iz;
	float x1 = x - 1.0f;
	float y1 = y - 1.0f;
	float z1 = z - 1.0f;

	ix &= 255;
	iy &= 255;
	iz &= 255;

	float s = qCurve(x);
	float t = qCurve(y);
	float u = qCurve(z);

	float dx = qCurveDeriv(x);
	float dy = qCurveDeriv(y);
	float dz = qCurveDeriv(z);

	// Hash coordinates of the 8 cube corners
	int p_a  = p[ix]    + iy;
	int p_aa = p[p_a]   + iz;
	int p_ab = p[p_a+1] + iz;
	int p_b  = p[ix+1]  + iy;
	int p_ba = p[p_b]   + iz;
	int p_bb = p[p_b+1] + iz;

	int a = p[p_aa];
	int b = p[p_ba];
	int c = p[p_ab];
	int d = p[p_bb];
	int e = p[p_aa + 1];
	int f = p[p_ba + 1];
	int g = p[p_ab + 1];
	int h = p[p_bb + 1];

	float k0 = static_cast<float>(a);
	float k1 = static_cast<float>(b - a);
	float k2 = static_cast<float>(c - a);
	float k3 = static_cast<float>(e - a);
	float k4 = static_cast<float>(a - b - c + d);
	float k5 = static_cast<float>(a - c - e + g);
	float k6 = static_cast<float>(a - b - e + f);
	float k7 = static_cast<float>(-a + b + c - d + e - f - g + h);

	out.n = k0 + k1*s + k2*t + k3*u + k4*s*t + k5*t*u + k6*s*u + k7*s*t*u;
	out.dx = dx * (k1 + k4*t + k6*u + k7*t*u);
	out.dy = dy * (k2 + k5*u + k4*s + k7*s*u);
	out.dz = dz * (k3 + k6*s + k5*t + k7*s*t);
	return out;
}


float noise::swissTurbulence(float x, float y, float z, int octaves,
							 float lacunarity, float persistence, float warp)
{
	float sum = 0.0f;
	float freq = 1.0f;
	float amp = 1.0f;
	float dxSum = 0.0f;
	float dySum = 0.0f;

	for (int i = 0; i < octaves; ++i) {
		float u = (x + warp * dxSum) * freq;
		float v = (y + warp * dySum) * freq;
		auto noise = perlinNoise3Deriv(u, v, z + i);
		sum += amp * (1.0f - abs(noise.n));
		dxSum += amp * noise.dx * -noise.n;
		dySum += amp * noise.dy * -noise.n;
		freq *= lacunarity;
		amp *= persistence * clamp(0.0f, 1.0f, sum);
	}
	return sum;
}


float noise::jordanTurbulence(float x, float y, float z,
							  int octaves, float lacunarity,
							  float gain1, float gain,
							  float warp0, float warp,
							  float damp0, float damp,
							  float damp_scale)
{
	auto noise = perlinNoise3Deriv(x, y, z);
	Noise3Deriv noise2{ noise.n*noise.n, noise.dx*noise.n, noise.dy*noise.n };
	float sum = noise2.n;
	
	float dxsum_warp = warp0*noise2.dx;
	float dysum_warp = warp0*noise2.dy;

	float dxsum_damp = damp0*noise2.dx;
	float dysum_damp = damp0*noise2.dy;

	float amp = gain1;
	float freq = lacunarity;
	float damped_amp = amp * gain;

	for (int i = 1; i < octaves; ++i) {
		float u = x * freq + dxsum_warp;
		float v = y * freq + dysum_warp;
		noise = perlinNoise3Deriv(u, v, z + i / 256.0f);
		noise2 = { noise.n*noise.n, noise.dx*noise.n, noise.dy*noise.n };
		sum += damped_amp * noise2.n;
		
		dxsum_warp += warp * noise2.dx;
		dysum_warp += warp * noise2.dy;
		
		dxsum_damp += damp * noise2.dx;
		dysum_damp += damp * noise2.dy;

		freq *= lacunarity;
		amp *= gain;
		damped_amp = amp * (1.0f - damp_scale / (1.0f + (dxsum_damp*dxsum_damp + dysum_damp*dysum_damp)));
	}
	return sum;
}
