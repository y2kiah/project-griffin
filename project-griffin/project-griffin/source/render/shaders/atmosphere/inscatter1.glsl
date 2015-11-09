#ifdef _GEOMETRY_
#extension GL_EXT_geometry_shader4 : enable
#endif

#include "source/render/shaders/layout.glsli"
#include "source/game/sky/atmosphere.h"
#include "source/render/shaders/atmosphere/common.glsli"

// computes single scattering (line 3 in algorithm 4.1)

uniform float r;
uniform vec4 dhdH;
uniform int layer;

#ifdef _VERTEX_

	// Input Variables

	layout(location = VertexLayout_Position) in vec3 vertexPosition;

	void main() {
		gl_Position = vec4(vertexPosition, 1.0);
	}

#endif

#ifdef _GEOMETRY_

	void main() {
		gl_Position = gl_PositionIn[0];
		gl_Layer = layer;
		EmitVertex();
		gl_Position = gl_PositionIn[1];
		gl_Layer = layer;
		EmitVertex();
		gl_Position = gl_PositionIn[2];
		gl_Layer = layer;
		EmitVertex();
		EndPrimitive();
	}

#endif

#ifdef _FRAGMENT_
	
	// Output Variables

	layout(location = 0) out vec4 outDeltaSR;
	layout(location = 1) out vec4 outDeltaSM;

	void integrand(float r, float mu, float muS, float nu, float t, out vec3 ray, out vec3 mie) {
		ray = vec3(0.0);
		mie = vec3(0.0);
		float ri = sqrt(r * r + t * t + 2.0 * r * mu * t);
		float muSi = (nu * t + muS * r) / ri;
		ri = max(Rg, ri);
		if (muSi >= -sqrt(1.0 - Rg * Rg / (ri * ri))) {
			vec3 ti = transmittance(r, mu, t) * transmittance(ri, muSi);
			ray = exp(-(ri - Rg) / H_rayleigh) * ti;
			mie = exp(-(ri - Rg) / H_mie) * ti;
		}
	}

	void inscatter(float r, float mu, float muS, float nu, out vec3 ray, out vec3 mie) {
		ray = vec3(0.0);
		mie = vec3(0.0);
		float dx = intersectAtmosphere(r, mu) / float(INSCATTER_INTEGRAL_SAMPLES);
		float xi = 0.0;
		vec3 rayi;
		vec3 miei;
		integrand(r, mu, muS, nu, 0.0, rayi, miei);
		for (int i = 1; i <= INSCATTER_INTEGRAL_SAMPLES; ++i) {
			float xj = float(i) * dx;
			vec3 rayj;
			vec3 miej;
			integrand(r, mu, muS, nu, xj, rayj, miej);
			ray += (rayi + rayj) / 2.0 * dx;
			mie += (miei + miej) / 2.0 * dx;
			xi = xj;
			rayi = rayj;
			miei = miej;
		}
		ray *= beta_rayleigh;
		mie *= beta_mieSca;
	}

	void main() {
		vec3 ray;
		vec3 mie;
		float mu, muS, nu;
		getMuMuSNu(r, dhdH, mu, muS, nu);
		inscatter(r, mu, muS, nu, ray, mie);
		// store separately Rayleigh and Mie contributions, WITHOUT the phase function factor
		// (cf "Angular precision")
		outDeltaSR.rgb = ray;
		outDeltaSM.rgb = mie;
	}

#endif
