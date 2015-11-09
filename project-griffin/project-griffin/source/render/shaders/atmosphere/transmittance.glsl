#include "source/render/shaders/layout.glsli"
#include "source/game/sky/atmosphere.h"
#include "source/render/shaders/atmosphere/common.glsli"

// computes transmittance table T using Eq (5)

#ifdef _VERTEX_
	
	// Input Variables

	layout(location = VertexLayout_Position) in vec3 vertexPosition;

	void main() {
		gl_Position = vec4(vertexPosition, 1.0);
	}

#endif

#ifdef _FRAGMENT_
	
	out vec4 outColor;

	/*
	// original
	float opticalDepth(float H, float r, float mu) {
		float result = 0.0;
		float dx = intersectAtmosphere(r, mu) / float(TRANSMITTANCE_INTEGRAL_SAMPLES);
		float yi = exp(-(r - Rg) / H);
		for (int i = 1; i <= TRANSMITTANCE_INTEGRAL_SAMPLES; ++i) {
			float xj = float(i) * dx;
			float alt_i = sqrt(r * r + xj * xj + 2.0 * xj * r * mu);
			float yj = exp(-(alt_i - Rg) / H);
			result += (yi + yj) / 2.0 * dx;
			yi = yj;
		}
		return mu < -sqrt(1.0 - (Rg / r) * (Rg / r)) ? 1e9 : result;
	}*/

	/**
	* density over path
	* scaleHeight is H_rayleigh or H_mie
	* r is 
	*/
	float opticalDepth(float scaleHeight, float r, float mu) {
		// if ray below horizon return max density
		float cosHorizon = -sqrt(1.0 - ((Rg * Rg) / (r * r)));
		if (mu < cosHorizon) {
			return 1e9;
		}
		float totalDensity = 0.0;
		float dx = intersectAtmosphere(r, mu) / float(TRANSMITTANCE_INTEGRAL_SAMPLES);
		float y_j = exp(-(r - Rg) / scaleHeight);
		for (int i = 1; i <= TRANSMITTANCE_INTEGRAL_SAMPLES; ++i) {
			float x_i = float(i) * dx;
			float alt_i = sqrt(r * r + x_i * x_i + 2.0 * x_i * r * mu);
			float y_i = exp(-(alt_i - Rg) / scaleHeight);
			totalDensity += (y_j + y_i) / 2.0 * dx;
			y_j = y_i;
		}
		return totalDensity;
	}

	/**
	* get values for:
	* r - radius (altitude between Rg and Rt)
	* muS - cosine of sun-zenith angle
	*/
	void getTransmittanceRMu(out float r, out float muS) {
		r = gl_FragCoord.y / float(TRANSMITTANCE_H);
		muS = gl_FragCoord.x / float(TRANSMITTANCE_W);
	#ifdef TRANSMITTANCE_NON_LINEAR
		r = Rg + (r * r) * (Rt - Rg);
		muS = -0.15 + tan(1.5 * muS) / tan(1.5) * (1.0 + 0.15);
	#else
		r = Rg + r * (Rt - Rg);
		muS = -0.15 + muS * (1.0 + 0.15);
	#endif
	}

	void main() {
		float r, muS;
		getTransmittanceRMu(r, muS);
	
		// calculates extinction factor of given altitude (r) and view-zenith angle (muS)
		vec3 depth = beta_rayleigh * opticalDepth(H_rayleigh, r, muS) +
					 beta_mieEx * opticalDepth(H_mie, r, muS);
				 
		outColor = vec4(exp(-depth), 0.0);
	}

#endif
