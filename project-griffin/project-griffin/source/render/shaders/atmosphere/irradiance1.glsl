#include "source/render/shaders/layout.glsli"
#include "source/game/sky/atmosphere.h"
#include "source/render/shaders/atmosphere/common.glsli"

// computes ground irradiance due to direct sunlight E[L0] (line 2 in algorithm 4.1)

#ifdef _VERTEX_

	void main() {
		gl_Position = gl_Vertex;
	}

#endif

#ifdef _FRAGMENT_

	void main() {
		float r, muS;
		getIrradianceRMuS(r, muS);
		gl_FragColor = vec4(transmittance(r, muS) * max(muS, 0.0), 0.0);
	}

#endif
