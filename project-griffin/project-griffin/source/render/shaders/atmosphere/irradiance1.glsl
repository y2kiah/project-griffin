#include "source/render/shaders/layout.glsli"
#include "source/game/sky/atmosphere.h"
#include "source/render/shaders/atmosphere/common.glsli"

// computes ground irradiance due to direct sunlight E[L0] (line 2 in algorithm 4.1)

#ifdef _VERTEX_
	
	// Input Variables

	layout(location = VertexLayout_Position) in vec3 vertexPosition;

	void main() {
		gl_Position = vec4(vertexPosition, 1.0);
	}

#endif

#ifdef _FRAGMENT_
	
	out vec4 outColor;

	void main() {
		float r, muS;
		getIrradianceRMuS(r, muS);
		outColor = vec4(transmittance(r, muS) * max(muS, 0.0), 0.0);
	}

#endif
