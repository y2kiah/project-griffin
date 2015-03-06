#include "source/render/shaders/layout.glsli"
#include "source/render/atmosphere/Main.h"
#include "source/render/shaders/atmosphere/common.glsli"

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