static const float EPSILON_ATMOSPHERE = 0.002f;
static const float EPSILON_INSCATTER = 0.004f;

Texture2D g_depth;
Texture2D g_color;
Texture2D g_normal;
Texture2D g_texIrradiance;
Texture3D g_texInscatter;

float3 g_cameraPos;
float3 g_sunVector;
float4x4 g_cameraWorld;
float4 g_frustumFar[4];
float4 g_frustumNear[4];
float sunIntensity = 30.0f;

struct VS_IN {
	float3 posL : POSITION;
	float2 texC : TEXCOORD0;
	uint index : TEXCOORD1;
};

struct VS_OUT {
	float4 posH : SV_POSITION;
	float2 texC : TEXCOORD0;
	float3 nearToFar : TEXCOORD2;
	float3 cameraToNear : TEXCOORD3;
};

SamplerState PointSamplerClamp {
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Clamp;
	AddressV = Clamp;
};

// vertex shader
VS_OUT VS(VS_IN input) {
	VS_OUT output;
	output.posH = float4(input.posL,1.0f);
	output.texC = input.texC;
	
	float3 frustumFarWorld = mul(float4(g_frustumFar[input.index].xyz, 1.0f), g_cameraWorld).xyz;
	float3 frustumNearWorld = mul(float4(g_frustumNear[input.index].xyz, 1.0f), g_cameraWorld).xyz;
	
	output.cameraToNear = frustumNearWorld - g_cameraPos;
	output.nearToFar = frustumFarWorld - frustumNearWorld;
	
	return output;
}

// input - d: view ray in world space
// output - offset: distance to atmosphere or 0 if within atmosphere
// output - maxPathLength: distance traversed within atmosphere
// output - return value: intersection occurred true/false
bool intersectAtmosphere(in float3 d, out float offset, out float maxPathLength) {
	offset = 0.0f;
	maxPathLength = 0.0f;
	
	// vector from ray origin to center of the sphere
	float3 l = -g_cameraPos;
	float l2 = dot(l,l);
	float s = dot(l,d);
	// adjust top atmosphere boundary by small epsilon to prevent artifacts
	float r = g_Rt - EPSILON_ATMOSPHERE;
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
// input/output - irradianceFactor: surface hit within atmosphere 1.0f
// otherwise 0.0f
// output - return value: total in-scattered light

float3 GetInscatteredLight(in float3 surfacePos, in float3 viewDir, in out float3 attenuation, in out float irradianceFactor) {
	float3 inscatteredLight = float3(0.0f, 0.0f, 0.0f);
	float offset;
	float maxPathLength;
	if (intersectAtmosphere(viewDir, offset, maxPathLength)) {

		float pathLength = distance(g_cameraPos, surfacePos);
		// check if object occludes atmosphere
		if (pathLength > offset) {
			// offsetting camera
			float3 startPos = g_cameraPos + offset * viewDir;
			float startPosHeight = length(startPos);
			pathLength -= offset;
		
			// starting position of path is now ensured to be inside atmosphere
			// was either originally there or has been moved to top boundary
			float muStartPos = dot(startPos, viewDir) / startPosHeight;
			float nuStartPos = dot(viewDir, g_sunVector);
			float musStartPos = dot(startPos, g_sunVector) / startPosHeight;
		
			// in-scattering for infinite ray (light in-scattered when
			// no surface hit or object behind atmosphere)
			float4 inscatter = max(texture4D(g_texInscatter, startPosHeight, muStartPos, musStartPos, nuStartPos), 0.0f);
			float surfacePosHeight = length(surfacePos);
			float musEndPos = dot(surfacePos, g_sunVector) / surfacePosHeight;
		
			// check if object hit is inside atmosphere
			if (pathLength < maxPathLength) {
				// reduce total in-scattered light when surface hit within atmosphere
				// fíx described in chapter 5.1.1
				attenuation = analyticTransmittance(startPosHeight, muStartPos, pathLength);
				float muEndPos = dot(surfacePos, viewDir) / surfacePosHeight;
				float4 inscatterSurface = texture4D(g_texInscatter, surfacePosHeight, muEndPos, musEndPos, nuStartPos);
				inscatter = max(inscatter-attenuation.rgbr*inscatterSurface, 0.0f);
				irradianceFactor = 1.0f;
			}
			else {
				// retrieve extinction factor for inifinte ray
				// fíx described in chapter 5.1.1
				attenuation = analyticTransmittance(startPosHeight, muStartPos, pathLength);
			}
		
			// avoids imprecision problems near horizon by interpolating between two points above and below horizon
			// fíx described in chapter 5.1.2
			float muHorizon = -sqrt(1.0 - (g_Rg / startPosHeight) * (g_Rg / startPosHeight));
			if (abs(muStartPos - muHorizon) < EPSILON_INSCATTER) {
				float mu = muHorizon - EPSILON_INSCATTER;
				float samplePosHeight = sqrt(startPosHeight*startPosHeight + pathLength*pathLength+2.0f*startPosHeight* pathLength*mu);
				float muSamplePos = (startPosHeight * mu + pathLength) / samplePosHeight;
				float4 inScatter0 = texture4D(g_texInscatter, startPosHeight, mu, musStartPos, nuStartPos);
				float4 inScatter1 = texture4D(g_texInscatter, samplePosHeight, muSamplePos, musEndPos, nuStartPos);
				float4 inScatterA = max(inScatter0-attenuation.rgbr*inScatter1,0.0);
			
				mu = muHorizon + EPSILON_INSCATTER;
				samplePosHeight = sqrt(startPosHeight*startPosHeight +pathLength*pathLength+2.0f* startPosHeight*pathLength*mu);
				muSamplePos = (startPosHeight * mu + pathLength) / samplePosHeight;
				inScatter0 = texture4D(g_texInscatter, startPosHeight, mu, musStartPos, nuStartPos);
				inScatter1 = texture4D(g_texInscatter, samplePosHeight, muSamplePos, musEndPos, nuStartPos);
			
				float4 inScatterB = max(inScatter0 - attenuation.rgbr * inScatter1, 0.0f);
				float t = ((muStartPos - muHorizon) + EPSILON_INSCATTER) / (2.0 * EPSILON_INSCATTER);
				inscatter = lerp(inScatterA, inScatterB, t);
			}
		
			// avoids imprecision problems in Mie scattering when sun is below horizon
			// fíx described in chapter 5.1.3
			inscatter.w *= smoothstep(0.00, 0.02, musStartPos);
			float phaseR = phaseFunctionR(nuStartPos);
			float phaseM = phaseFunctionM(nuStartPos);
			inscatteredLight = max(inscatter.rgb * phaseR + getMie(inscatter)* phaseM, 0.0f);
			inscatteredLight *= sunIntensity;
		}
	}
	return inscatteredLight;
}

// input - surfacePos: reconstructed position of current pixel
// input - texC: texture coordinates
// input - attenuation: extinction factor along view path
// input - irradianceFactor: surface hit within atmosphere 1.0f otherwise 0.0f
// output - return value: total reflected light + direct sunlight
float3 GetReflectedLight(in float3 surfacePos, in float2 texC, in float3 attenuation, in float irradianceFactor) {
	// read contents of GBuffer
	float4 normalData = g_normal.SampleLevel(PointSamplerClamp, texC, 0);
	float3 surfaceColor = g_color.SampleLevel(PointSamplerClamp, texC, 0).rgb;
	
	// decode normal and determine intensity of refected light at surface postiion
	float3 normal = 2.0f * normalData.xyz - 1.0f;
	float lightIntensity = sunIntensity * normalData.w;
	float lightScale = max(dot(normal, g_sunVector), 0.0f);
	
	// irradiance at surface position due to sky light
	float surfacePosHeight = length(surfacePos);
	float musSurfacePos = dot(surfacePos, g_sunVector) / surfacePosHeight;
	float3 irradianceSurface = irradiance(g_texIrradiance, surfacePosHeight, musSurfacePos) * irradianceFactor;
	
	// attenuate direct sun light on its path from top of atmosphere to surface position
	float3 attenuationSunLight = transmittance(surfacePosHeight,musSurfacePos);
	float3 reflectedLight = surfaceColor * (lightScale * attenuationSunLight + irradianceSurface) * lightIntensity;
	
	// attenuate again on path from surface position to camera
	reflectedLight *= attenuation;
	
	return reflectedLight;
}

// pixel shader
float4 PS_PLANET_DEFERRED(VS_OUT input) : SV_TARGET0 {
	// reconstructing world space postion by interpolation
	float depthVal = g_depth.SampleLevel( PointSamplerClamp, input.texC, 0 ).r;
	float3 surfacePos = g_cameraPos + input.cameraToNear + depthVal * input.nearToFar;
	
	// obtaining the view direction vector float3 viewDir = normalize(input.nearToFar);
	float3 attenuation = float3(1.0f, 1.0f, 1.0f);
	float irradianceFactor = 0.0f;
	float3 inscatteredLight = GetInscatteredLight(surfacePos, viewDir, attenuation, irradianceFactor);
	float3 reflectedLight = GetReflectedLight(surfacePos, input.texC, attenuation, irradianceFactor);
	
	return float4(HDR(reflectedLight + inscatteredLight), 1.0f);
}

technique10 RenderPlanetDeferred {
	pass P0 {
		SetVertexShader( CompileShader( vs_4_0, VS() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_4_0, PS_PLANET_DEFERRED() ) );
	}
}