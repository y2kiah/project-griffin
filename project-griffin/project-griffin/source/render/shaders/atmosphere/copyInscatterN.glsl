#ifdef _GEOMETRY_
#extension GL_EXT_geometry_shader4 : enable
#endif

#include "source/render/shaders/layout.glsli"
#include "source/game/sky/atmosphere.h"
#include "source/render/shaders/atmosphere/common.glsli"

// adds deltaS into S (line 11 in algorithm 4.1)

uniform float r;
uniform vec4 dhdH;
uniform int layer;

uniform sampler3D deltaSSampler;

#ifdef _VERTEX_

	const vec2 madd = vec2(0.5, 0.5);

	layout(location = VertexLayout_Position) in vec3 vertexPosition;

	out vec2 uv;

	void main() {
		gl_Position = vec4(vertexPosition, 1.0);
		uv = vertexPosition.xy * madd + madd;
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
	
	out vec4 outColor;

	in vec2 uv;

	void main() {
		float mu, muS, nu;
		getMuMuSNu(r, dhdH, mu, muS, nu);
		vec3 uvw = vec3(uv, float(layer) + 0.5) / vec3(ivec3(RES_MU_S * RES_NU, RES_MU, RES_R));
		outColor = vec4(texture(deltaSSampler, uvw).rgb / phaseFunctionR(nu), 0.0);
	}

#endif
