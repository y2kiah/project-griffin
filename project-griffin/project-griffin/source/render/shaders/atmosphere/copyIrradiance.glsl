#include "source/render/shaders/layout.glsli"
#include "source/game/sky/atmosphere.h"
#include "source/render/shaders/atmosphere/common.glsli"

// copies deltaE into E (lines 4 and 10 in algorithm 4.1)

uniform float k; // k=0 for line 4, k=1 for line 10
uniform sampler2D deltaESampler;

#ifdef _VERTEX_

	void main() {
		gl_Position = gl_Vertex;
	}

#endif

#ifdef _FRAGMENT_

	void main() {
		vec2 uv = gl_FragCoord.xy / vec2(SKY_W, SKY_H);
		gl_FragColor = k * texture2D(deltaESampler, uv); // k=0 for line 4, k=1 for line 10
	}

#endif
