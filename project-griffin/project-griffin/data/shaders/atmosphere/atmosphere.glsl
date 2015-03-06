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
/**
 * Precomputed Atmospheric Scattering
 * Copyright (c) 2008 INRIA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * Author: Eric Bruneton
 */

const float Rg = 6360.0; // radius at ground
const float Rt = 6420.0; // radius at top of atmosphere
const float RL = 6421.0;
//const float Rg = 3180.0;
//const float Rt = 3220.0;
//const float RL = 3221.0;

const int TRANSMITTANCE_W = 256;
const int TRANSMITTANCE_H = 64;

const int SKY_W = 64; // changing these seems to have no impact, why?
const int SKY_H = 16;

const int RES_R = 32;
const int RES_MU = 128;
const int RES_MU_S = 32;
const int RES_NU = 8;

/**
 * Precomputed Atmospheric Scattering
 * Copyright (c) 2008 INRIA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * Author: Eric Bruneton
 */

// ----------------------------------------------------------------------------
// PHYSICAL MODEL PARAMETERS
// ----------------------------------------------------------------------------

const float AVERAGE_GROUND_REFLECTANCE = 0.1;

// Rayleigh
const float H_rayleigh = 8.0;
const vec3 beta_rayleigh = vec3(5.8e-3, 1.35e-2, 3.31e-2);

// Mie
// DEFAULT
const float H_mie = 1.2;
const vec3 beta_mieSca = vec3(4e-3);
const vec3 beta_mieEx = beta_mieSca / 0.9;
const float mieG = 0.8;
// CLEAR SKY
/*const float H_mie = 1.2;
const vec3 beta_mieSca = vec3(20e-3);
const vec3 beta_mieEx = beta_mieSca / 0.9;
const float mieG = 0.76;*/
// PARTLY CLOUDY
/*const float H_mie = 3.0;
const vec3 beta_mieSca = vec3(3e-3);
const vec3 beta_mieEx = beta_mieSca / 0.9;
const float mieG = 0.65;*/

// ----------------------------------------------------------------------------
// NUMERICAL INTEGRATION PARAMETERS
// ----------------------------------------------------------------------------

const int TRANSMITTANCE_INTEGRAL_SAMPLES = 1000;//500;
const int INSCATTER_INTEGRAL_SAMPLES = 100;//50;
const int IRRADIANCE_INTEGRAL_SAMPLES = 64;//32;
const int INSCATTER_SPHERICAL_INTEGRAL_SAMPLES = 32;//16;

const float M_PI = 3.1415926535897932384626433832795;

// ----------------------------------------------------------------------------
// PARAMETERIZATION OPTIONS
// ----------------------------------------------------------------------------

#define TRANSMITTANCE_NON_LINEAR
#define INSCATTER_NON_LINEAR

// ----------------------------------------------------------------------------
// PARAMETERIZATION FUNCTIONS
// ----------------------------------------------------------------------------

uniform sampler2D transmittanceSampler;

vec2 getTransmittanceUV(float r, float mu) {
    float uR, uMu;
#ifdef TRANSMITTANCE_NON_LINEAR
	uR = sqrt((r - Rg) / (Rt - Rg));
	uMu = atan((mu + 0.15) / (1.0 + 0.15) * tan(1.5)) / 1.5;
#else
	uR = (r - Rg) / (Rt - Rg);
	uMu = (mu + 0.15) / (1.0 + 0.15);
#endif
    return vec2(uMu, uR);
}

vec2 getIrradianceUV(float r, float muS) {
    float uR = (r - Rg) / (Rt - Rg);
    float uMuS = (muS + 0.2) / (1.0 + 0.2);
    return vec2(uMuS, uR);
}

#ifdef _FRAGMENT_
void getIrradianceRMuS(out float r, out float muS) {
    r = Rg + (gl_FragCoord.y - 0.5) / (float(SKY_H) - 1.0) * (Rt - Rg);
    muS = -0.2 + (gl_FragCoord.x - 0.5) / (float(SKY_W) - 1.0) * (1.0 + 0.2);
}
#endif

vec4 texture4D(sampler3D table, float r, float mu, float muS, float nu)
{
    float H = sqrt(Rt * Rt - Rg * Rg);
    float rho = sqrt(r * r - Rg * Rg);
#ifdef INSCATTER_NON_LINEAR
    float rmu = r * mu;
    float delta = rmu * rmu - r * r + Rg * Rg;
    vec4 cst = rmu < 0.0 && delta > 0.0 ? vec4(1.0, 0.0, 0.0, 0.5 - 0.5 / float(RES_MU)) : vec4(-1.0, H * H, H, 0.5 + 0.5 / float(RES_MU));
	float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R));
    float uMu = cst.w + (rmu * cst.x + sqrt(delta + cst.y)) / (rho + cst.z) * (0.5 - 1.0 / float(RES_MU));
    // paper formula
    //float uMuS = 0.5 / float(RES_MU_S) + max((1.0 - exp(-3.0 * muS - 0.6)) / (1.0 - exp(-3.6)), 0.0) * (1.0 - 1.0 / float(RES_MU_S));
    // better formula
    float uMuS = 0.5 / float(RES_MU_S) + (atan(max(muS, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26)) * 0.5 * (1.0 - 1.0 / float(RES_MU_S));
#else
	float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R));
    float uMu = 0.5 / float(RES_MU) + (mu + 1.0) / 2.0 * (1.0 - 1.0 / float(RES_MU));
    float uMuS = 0.5 / float(RES_MU_S) + max(muS + 0.2, 0.0) / 1.2 * (1.0 - 1.0 / float(RES_MU_S));
#endif
    float lerp = (nu + 1.0) / 2.0 * (float(RES_NU) - 1.0);
    float uNu = floor(lerp);
    lerp = lerp - uNu;
    return texture(table, vec3((uNu + uMuS) / float(RES_NU), uMu, uR)) * (1.0 - lerp) +
           texture(table, vec3((uNu + uMuS + 1.0) / float(RES_NU), uMu, uR)) * lerp;
}

#ifdef _FRAGMENT_
void getMuMuSNu(float r, vec4 dhdH, out float mu, out float muS, out float nu) {
    float x = gl_FragCoord.x - 0.5;
    float y = gl_FragCoord.y - 0.5;
#ifdef INSCATTER_NON_LINEAR
    if (y < float(RES_MU) / 2.0) {
        float d = 1.0 - y / (float(RES_MU) / 2.0 - 1.0);
        d = min(max(dhdH.z, d * dhdH.w), dhdH.w * 0.999);
        mu = (Rg * Rg - r * r - d * d) / (2.0 * r * d);
        mu = min(mu, -sqrt(1.0 - (Rg / r) * (Rg / r)) - 0.001);
    } else {
        float d = (y - float(RES_MU) / 2.0) / (float(RES_MU) / 2.0 - 1.0);
        d = min(max(dhdH.x, d * dhdH.y), dhdH.y * 0.999);
        mu = (Rt * Rt - r * r - d * d) / (2.0 * r * d);
    }
    muS = mod(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0);
    // paper formula
    //muS = -(0.6 + log(1.0 - muS * (1.0 -  exp(-3.6)))) / 3.0;
    // better formula
    muS = tan((2.0 * muS - 1.0 + 0.26) * 1.1) / tan(1.26 * 1.1);
    nu = -1.0 + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0) * 2.0;
#else
    mu = -1.0 + 2.0 * y / (float(RES_MU) - 1.0);
    muS = mod(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0);
    muS = -0.2 + muS * 1.2;
    nu = -1.0 + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0) * 2.0;
#endif
}
#endif

// ----------------------------------------------------------------------------
// UTILITY FUNCTIONS
// ----------------------------------------------------------------------------

// intersect atmosphere
// nearest intersection of ray r,mu with ground or top atmosphere boundary
// mu=cos(ray zenith angle at ray origin)
float intersectAtmosphere(float r, float mu) {
    float dout = -r * mu + sqrt(r * r * (mu * mu - 1.0) + RL * RL);
    float delta2 = r * r * (mu * mu - 1.0) + Rg * Rg;
    if (delta2 >= 0.0) {
        float din = -r * mu - sqrt(delta2);
        if (din >= 0.0) {
            dout = min(dout, din);
        }
    }
    return dout;
}

// transmittance(=transparency) of atmosphere for infinite ray (r,mu)
// (mu=cos(view zenith angle)), intersections with ground ignored
vec3 transmittance(float r, float mu) {
	vec2 uv = getTransmittanceUV(r, mu);
    return texture2D(transmittanceSampler, uv).rgb;
}

// transmittance(=transparency) of atmosphere for infinite ray (r,mu)
// (mu=cos(view zenith angle)), or zero if ray intersects ground
vec3 transmittanceWithShadow(float r, float mu) {
    return mu < -sqrt(1.0 - (Rg / r) * (Rg / r)) ? vec3(0.0) : transmittance(r, mu);
}

// transmittance(=transparency) of atmosphere between x and x0
// assume segment x,x0 not intersecting ground
// r=||x||, mu=cos(zenith angle of [x,x0) ray at x), v=unit direction vector of [x,x0) ray
vec3 transmittance(float r, float mu, vec3 v, vec3 x0) {
    vec3 result;
    float r1 = length(x0);
    float mu1 = dot(x0, v) / r;
    if (mu > 0.0) {
        result = min(transmittance(r, mu) / transmittance(r1, mu1), 1.0);
    } else {
        result = min(transmittance(r1, -mu1) / transmittance(r, -mu), 1.0);
    }
    return result;
}

// optical depth for ray (r,mu) of length d, using analytic formula
// (mu=cos(view zenith angle)), intersections with ground ignored
// H=height scale of exponential density function
float opticalDepth(float H, float r, float mu, float d) {
    float a = sqrt((0.5/H)*r);
    vec2 a01 = a*vec2(mu, mu + d / r);
    vec2 a01s = sign(a01);
    vec2 a01sq = a01*a01;
    float x = a01s.y > a01s.x ? exp(a01sq.x) : 0.0;
    vec2 y = a01s / (2.3193*abs(a01) + sqrt(1.52*a01sq + 4.0)) * vec2(1.0, exp(-d/H*(d/(2.0*r)+mu)));
    return sqrt((6.2831*H)*r) * exp((Rg-r)/H) * (x + dot(y, vec2(1.0, -1.0)));
}

// transmittance(=transparency) of atmosphere for ray (r,mu) of length d
// (mu=cos(view zenith angle)), intersections with ground ignored
// uses analytic formula instead of transmittance texture
vec3 analyticTransmittance(float r, float mu, float d) {
    return exp(-beta_rayleigh * opticalDepth(H_rayleigh, r, mu, d) - beta_mieEx * opticalDepth(H_mie, r, mu, d));
}

// transmittance(=transparency) of atmosphere between x and x0
// assume segment x,x0 not intersecting ground
// d = distance between x and x0, mu=cos(zenith angle of [x,x0) ray at x)
vec3 transmittance(float r, float mu, float d) {
    vec3 result;
    float r1 = sqrt(r * r + d * d + 2.0 * r * mu * d);
    float mu1 = (r * mu + d) / r1;
    if (mu > 0.0) {
        result = min(transmittance(r, mu) / transmittance(r1, mu1), 1.0);
    } else {
        result = min(transmittance(r1, -mu1) / transmittance(r, -mu), 1.0);
    }
    return result;
}

vec3 irradiance(sampler2D sampler, float r, float muS) {
    vec2 uv = getIrradianceUV(r, muS);
    return texture2D(sampler, uv).rgb;
}

// Rayleigh phase function
float phaseFunctionR(float mu) {
    return (3.0 / (16.0 * M_PI)) * (1.0 + mu * mu);
}

// Mie phase function
float phaseFunctionM(float mu) {
	return 1.5 * 1.0 / (4.0 * M_PI) * (1.0 - mieG*mieG) * pow(1.0 + (mieG*mieG) - 2.0*mieG*mu, -3.0/2.0) * (1.0 + mu * mu) / (2.0 + mieG*mieG);
}

// approximated single Mie scattering (cf. approximate Cm in paragraph "Angular precision")
vec3 getMie(vec4 rayMie) { // rayMie.rgb=C*, rayMie.w=Cm,r
	return rayMie.rgb * rayMie.w / max(rayMie.r, 1e-4) * (beta_rayleigh.r / beta_rayleigh);
}


const float EPSILON_ATMOSPHERE = 0.002;
const float EPSILON_INSCATTER = 0.004;
const float sunIntensity = 30.0;

uniform vec3 g_cameraPos;
uniform vec3 g_sunVector;
uniform mat4 g_cameraWorld;
uniform vec4 g_frustumFar[4];
uniform vec4 g_frustumNear[4];

#ifdef _VERTEX_

	layout(location = VertexLayout_Position) in vec3 vertexPosition;
	layout(location = VertexLayout_TextureCoords) in vec2 vertexUV;

	in uint index;

	out vec2 texC;
	out vec3 nearToFar;
	out vec3 cameraToNear;

	// vertex shader
	void main()
	{
		gl_Position = vec4(vertexPosition, 1.0);
		texC = vertexUV;
		
		vec3 frustumFarWorld = (g_cameraWorld * vec4(g_frustumFar[index].xyz, 1.0)).xyz;
		vec3 frustumNearWorld = (g_cameraWorld * vec4(g_frustumNear[index].xyz, 1.0)).xyz;
		
		cameraToNear = frustumNearWorld - g_cameraPos;
		nearToFar = frustumFarWorld - frustumNearWorld;
	}

#endif

#ifdef _FRAGMENT_
	
	uniform sampler2D g_depth;
	uniform sampler2D g_color;
	uniform sampler2D g_normal;
	uniform sampler2D g_texIrradiance;
	uniform sampler3D g_texInscatter;

	in vec2 texC;
	in vec3 nearToFar;
	in vec3 cameraToNear;

	out vec4 outColor;

	// input - d: view ray in world space
	// output - offset: distance to atmosphere or 0 if within atmosphere
	// output - maxPathLength: distance traversed within atmosphere
	// output - return value: intersection occurred true/false
	bool intersectAtmosphere(in vec3 d, out float offset, out float maxPathLength) {
		offset = 0.0;
		maxPathLength = 0.0;
	
		// vector from ray origin to center of the sphere
		vec3 l = -g_cameraPos;
		float l2 = dot(l,l);
		float s = dot(l,d);
		// adjust top atmosphere boundary by small epsilon to prevent artifacts
		float r = Rt - EPSILON_ATMOSPHERE;
		float r2 = r*r;
	
		if (l2 <= r2) {
			// ray origin inside sphere, hit is ensured
			float m2 = l2 - (s * s);
			float q = sqrt(r2 - m2);
			maxPathLength = s + q;
		
			return true;
		}
		else if (s >= 0) {
			// ray starts outside in front of sphere, hit is possible
			float m2 = l2 - (s * s);
			if (m2 <= r2) {
				// ray hits atmosphere definitely
				float q = sqrt(r2 - m2);
				offset = s - q;
				maxPathLength = (s + q) - offset;
				return true;
			}
		}
	
		return false;
	}

	// input - surfacePos: reconstructed position of current pixel
	// input - viewDir: view ray in world space
	// input/output - attenuation: extinction factor along view path
	// input/output - irradianceFactor: surface hit within atmosphere 1.0
	// otherwise 0.0f
	// output - return value: total in-scattered light
	vec3 getInscatteredLight(in vec3 surfacePos, in vec3 viewDir, in out vec3 attenuation, in out float irradianceFactor) {
		vec3 inscatteredLight = vec3(0.0, 0.0, 0.0);
		float offset;
		float maxPathLength;
		if (intersectAtmosphere(viewDir, offset, maxPathLength)) {

			float pathLength = distance(g_cameraPos, surfacePos);
			// check if object occludes atmosphere
			if (pathLength > offset) {
				// offsetting camera
				vec3 startPos = g_cameraPos + offset * viewDir;
				float startPosHeight = length(startPos);
				pathLength -= offset;
		
				// starting position of path is now ensured to be inside atmosphere
				// was either originally there or has been moved to top boundary
				float muStartPos = dot(startPos, viewDir) / startPosHeight;
				float nuStartPos = dot(viewDir, g_sunVector);
				float musStartPos = dot(startPos, g_sunVector) / startPosHeight;
		
				// in-scattering for infinite ray (light in-scattered when
				// no surface hit or object behind atmosphere)
				vec4 inscatter = max(texture4D(g_texInscatter, startPosHeight, muStartPos, musStartPos, nuStartPos), 0.0);
				float surfacePosHeight = length(surfacePos);
				float musEndPos = dot(surfacePos, g_sunVector) / surfacePosHeight;
		
				// check if object hit is inside atmosphere
				if (pathLength < maxPathLength) {
					// reduce total in-scattered light when surface hit within atmosphere
					// fíx described in chapter 5.1.1
					attenuation = analyticTransmittance(startPosHeight, muStartPos, pathLength);
					float muEndPos = dot(surfacePos, viewDir) / surfacePosHeight;
					vec4 inscatterSurface = texture4D(g_texInscatter, surfacePosHeight, muEndPos, musEndPos, nuStartPos);
					inscatter = max(inscatter-attenuation.rgbr*inscatterSurface, 0.0);
					irradianceFactor = 1.0;
				}
				else {
					// retrieve extinction factor for inifinte ray
					// fíx described in chapter 5.1.1
					attenuation = analyticTransmittance(startPosHeight, muStartPos, pathLength);
				}
		
				// avoids imprecision problems near horizon by interpolating between two points above and below horizon
				// fíx described in chapter 5.1.2
				float muHorizon = -sqrt(1.0 - (Rg / startPosHeight) * (Rg / startPosHeight));
				if (abs(muStartPos - muHorizon) < EPSILON_INSCATTER) {
					float mu = muHorizon - EPSILON_INSCATTER;
					float samplePosHeight = sqrt(startPosHeight*startPosHeight + pathLength*pathLength+2.0*startPosHeight* pathLength*mu);
					float muSamplePos = (startPosHeight * mu + pathLength) / samplePosHeight;
					vec4 inScatter0 = texture4D(g_texInscatter, startPosHeight, mu, musStartPos, nuStartPos);
					vec4 inScatter1 = texture4D(g_texInscatter, samplePosHeight, muSamplePos, musEndPos, nuStartPos);
					vec4 inScatterA = max(inScatter0-attenuation.rgbr*inScatter1,0.0);
			
					mu = muHorizon + EPSILON_INSCATTER;
					samplePosHeight = sqrt(startPosHeight*startPosHeight +pathLength*pathLength+2.0* startPosHeight*pathLength*mu);
					muSamplePos = (startPosHeight * mu + pathLength) / samplePosHeight;
					inScatter0 = texture4D(g_texInscatter, startPosHeight, mu, musStartPos, nuStartPos);
					inScatter1 = texture4D(g_texInscatter, samplePosHeight, muSamplePos, musEndPos, nuStartPos);
			
					vec4 inScatterB = max(inScatter0 - attenuation.rgbr * inScatter1, 0.0);
					float t = ((muStartPos - muHorizon) + EPSILON_INSCATTER) / (2.0 * EPSILON_INSCATTER);
					inscatter = mix(inScatterA, inScatterB, t);
				}
		
				// avoids imprecision problems in Mie scattering when sun is below horizon
				// fíx described in chapter 5.1.3
				inscatter.w *= smoothstep(0.00, 0.02, musStartPos);
				float phaseR = phaseFunctionR(nuStartPos);
				float phaseM = phaseFunctionM(nuStartPos);
				inscatteredLight = max(inscatter.rgb * phaseR + getMie(inscatter)* phaseM, 0.0);
				inscatteredLight *= sunIntensity;
			}
		}
		return inscatteredLight;
	}

	// input - surfacePos: reconstructed position of current pixel
	// input - texC: texture coordinates
	// input - attenuation: extinction factor along view path
	// input - irradianceFactor: surface hit within atmosphere 1.0 otherwise 0.0
	// output - return value: total reflected light + direct sunlight
	vec3 getReflectedLight(in vec3 surfacePos, in vec2 texC, in vec3 attenuation, in float irradianceFactor) {
		// read contents of GBuffer
		vec4 normalData = textureLod(g_normal, texC, 0);      // PointSamplerClamp
		vec3 surfaceColor = textureLod(g_color, texC, 0).rgb; // PointSamplerClamp
	
		// decode normal and determine intensity of refected light at surface postiion
		vec3 normal = 2.0 * normalData.xyz - 1.0;
		float lightIntensity = sunIntensity * normalData.w;
		float lightScale = max(dot(normal, g_sunVector), 0.0);
	
		// irradiance at surface position due to sky light
		float surfacePosHeight = length(surfacePos);
		float musSurfacePos = dot(surfacePos, g_sunVector) / surfacePosHeight;
		vec3 irradianceSurface = irradiance(g_texIrradiance, surfacePosHeight, musSurfacePos) * irradianceFactor;
	
		// attenuate direct sun light on its path from top of atmosphere to surface position
		vec3 attenuationSunLight = transmittance(surfacePosHeight,musSurfacePos);
		vec3 reflectedLight = surfaceColor * (lightScale * attenuationSunLight + irradianceSurface) * lightIntensity;
	
		// attenuate again on path from surface position to camera
		reflectedLight *= attenuation;
	
		return reflectedLight;
	}


	const float EXPOSURE = 2.0;

	vec3 HDR(vec3 color) {
		return 1.0 - exp(-EXPOSURE * color);
	}

	// for reference, this is part of sampler state in gl
	/*SamplerState PointSamplerClamp {
		Filter = MIN_MAG_MIP_POINT;
		AddressU = Clamp;
		AddressV = Clamp;
	};*/

	void main()
	{
		// reconstructing world space postion by interpolation
		float depthVal = textureLod(g_depth, texC, 0).r; // PointSamplerClamp
		vec3 surfacePos = g_cameraPos + cameraToNear + depthVal * nearToFar;
	
		// obtaining the view direction vector
		vec3 viewDir = normalize(nearToFar);
		vec3 attenuation = vec3(1.0, 1.0, 1.0);
		float irradianceFactor = 0.0;
		vec3 inscatteredLight = getInscatteredLight(surfacePos, viewDir, attenuation, irradianceFactor);
		vec3 reflectedLight = getReflectedLight(surfacePos, texC, attenuation, irradianceFactor);
	
		outColor = vec4(HDR(reflectedLight + inscatteredLight), 1.0);
	}

#endif