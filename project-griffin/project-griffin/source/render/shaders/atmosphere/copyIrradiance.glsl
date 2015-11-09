#include "source/render/shaders/layout.glsli"
#include "source/game/sky/atmosphere.h"
#include "source/render/shaders/atmosphere/common.glsli"

// copies deltaE into E (lines 4 and 10 in algorithm 4.1)

uniform float k; // k=0 for line 4, k=1 for line 10
uniform sampler2D deltaESampler;

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
		vec2 uv = gl_FragCoord.xy / vec2(SKY_W, SKY_H);
		outColor = k * texture(deltaESampler, uv); // k=0 for line 4, k=1 for line 10
	}

#endif
