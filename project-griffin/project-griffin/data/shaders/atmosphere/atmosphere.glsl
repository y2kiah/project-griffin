// ================================================================================================
// from Main.h
// ================================================================================================

const float g_Rg = 6360.0; // radius at ground
const float g_Rt = 6420.0; // radius at top of atmosphere
const float g_RL = 6421.0;

const int RES_R = 32;
const int RES_MU = 128;
const int RES_MU_S = 32;
const int RES_NU = 8;

// ================================================================================================

const float EPSILON_ATMOSPHERE = 0.002;
const float EPSILON_INSCATTER = 0.004;
const float sunIntensity = 30.0;

uniform vec3 g_cameraPos;
uniform vec3 g_sunVector;
uniform mat4 g_cameraWorld;
uniform vec4 g_frustumFar[4];
uniform vec4 g_frustumNear[4];

#ifdef _VERTEX_

	in vec3 posL;
	in vec2 uv;
	in uint index;

	out vec2 texC;
	out vec3 nearToFar;
	out vec3 cameraToNear;

	// vertex shader
	void main()
	{
		gl_Position = vec4(posL,1.0);
		texC = uv;
		
		vec3 frustumFarWorld = mul(vec4(g_frustumFar[index].xyz, 1.0), g_cameraWorld).xyz;
		vec3 frustumNearWorld = mul(vec4(g_frustumNear[index].xyz, 1.0), g_cameraWorld).xyz;
		
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

	// ============================================================================================
	// from common.glsl
	// ============================================================================================

	// --------------------------------------------------------------------------------------------
	// PHYSICAL MODEL PARAMETERS
	// --------------------------------------------------------------------------------------------

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

	// ----------------------------------------------------------------------------
	// NUMERICAL INTEGRATION PARAMETERS
	// ----------------------------------------------------------------------------

	const int TRANSMITTANCE_INTEGRAL_SAMPLES = 500;
	const int INSCATTER_INTEGRAL_SAMPLES = 50;
	const int IRRADIANCE_INTEGRAL_SAMPLES = 32;
	const int INSCATTER_SPHERICAL_INTEGRAL_SAMPLES = 16;
	
	const float M_PI = 3.1415926535897932384626433832795;


	// ----------------------------------------------------------------------------
	// PARAMETERIZATION FUNCTIONS
	// ----------------------------------------------------------------------------

	uniform sampler2D transmittanceSampler;

	vec2 getTransmittanceUV(float r, float mu) {
		float uR, uMu;
		uR = sqrt((r - g_Rg) / (g_Rt - g_Rg));
		uMu = atan((mu + 0.15) / (1.0 + 0.15) * tan(1.5)) / 1.5;
		return vec2(uMu, uR);
	}

	vec2 getIrradianceUV(float r, float muS) {
		float uR = (r - g_Rg) / (g_Rt - g_Rg);
		float uMuS = (muS + 0.2) / (1.0 + 0.2);
		return vec2(uMuS, uR);
	}

	vec4 texture4D(sampler3D table, float r, float mu, float muS, float nu)
	{
		float H = sqrt(g_Rt * g_Rt - g_Rg * g_Rg);
		float rho = sqrt(r * r - g_Rg * g_Rg);

		float rmu = r * mu;
		float delta = rmu * rmu - r * r + g_Rg * g_Rg;
		vec4 cst = rmu < 0.0 && delta > 0.0 ? vec4(1.0, 0.0, 0.0, 0.5 - 0.5 / float(RES_MU)) : vec4(-1.0, H * H, H, 0.5 + 0.5 / float(RES_MU));
		float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R));
		float uMu = cst.w + (rmu * cst.x + sqrt(delta + cst.y)) / (rho + cst.z) * (0.5 - 1.0 / float(RES_MU));
		// paper formula
		//float uMuS = 0.5 / float(RES_MU_S) + max((1.0 - exp(-3.0 * muS - 0.6)) / (1.0 - exp(-3.6)), 0.0) * (1.0 - 1.0 / float(RES_MU_S));
		// better formula
		float uMuS = 0.5 / float(RES_MU_S) + (atan(max(muS, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26)) * 0.5 * (1.0 - 1.0 / float(RES_MU_S));
		
		float lerp = (nu + 1.0) / 2.0 * (float(RES_NU) - 1.0);
		float uNu = floor(lerp);
		lerp = lerp - uNu;
		return texture3D(table, vec3((uNu + uMuS) / float(RES_NU), uMu, uR)) * (1.0 - lerp) +
			   texture3D(table, vec3((uNu + uMuS + 1.0) / float(RES_NU), uMu, uR)) * lerp;
	}

	// ----------------------------------------------------------------------------
	// UTILITY FUNCTIONS
	// ----------------------------------------------------------------------------

	// intersect atmosphere
	// nearest intersection of ray r,mu with ground or top atmosphere boundary
	// mu=cos(ray zenith angle at ray origin)
	float intersectAtmosphere(float r, float mu) {
		float dout = -r * mu + sqrt(r * r * (mu * mu - 1.0) + g_RL * g_RL);
		float delta2 = r * r * (mu * mu - 1.0) + g_Rg * g_Rg;
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
		return sqrt((6.2831*H)*r) * exp((g_Rg-r)/H) * (x + dot(y, vec2(1.0, -1.0)));
	}

	// transmittance(=transparency) of atmosphere for ray (r,mu) of length d
	// (mu=cos(view zenith angle)), intersections with ground ignored
	// uses analytic formula instead of transmittance texture
	vec3 analyticTransmittance(float r, float mu, float d) {
		return exp(-beta_rayleigh * opticalDepth(H_rayleigh, r, mu, d) - beta_mieEx * opticalDepth(H_mie, r, mu, d));
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


	//=============================================================================================

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
				float muHorizon = -sqrt(1.0 - (g_Rg / startPosHeight) * (g_Rg / startPosHeight));
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