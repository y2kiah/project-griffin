#ifdef _GEOMETRY_
#extension GL_EXT_geometry_shader4 : enable
#endif

#include "source/render/shaders/layout.glsli"
#include "source/game/sky/atmosphere.h"
#include "source/render/shaders/atmosphere/common.glsli"

// copies deltaS into S (line 5 in algorithm 4.1)

uniform float r;
uniform vec4 dhdH;
uniform int layer;

uniform sampler3D deltaSRSampler;
uniform sampler3D deltaSMSampler;

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
	
	out vec4 outColor;

	void main() {
		vec3 uvw = vec3(gl_FragCoord.xy, float(layer) + 0.5) / vec3(ivec3(RES_MU_S * RES_NU, RES_MU, RES_R));
		vec4 ray = texture(deltaSRSampler, uvw);
		vec4 mie = texture(deltaSMSampler, uvw);
		outColor = vec4(ray.rgb, mie.r); // store only red component of single Mie scattering (cf. "Angular precision")
	}

#endif
