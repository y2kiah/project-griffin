#include "source/render/shaders/layout.glsli"

#ifdef _VERTEX_
	
	uniform mat4 modelViewProjection;

	layout(location = VertexLayout_Position) in vec3 vertexPosition_modelspace;

	out vec3 uvw;

	void main()
	{
		uvw = vertexPosition_modelspace;
		gl_Position = modelViewProjection * vec4(vertexPosition_modelspace, 1.0);
	}
	
#endif

#ifdef _FRAGMENT_

	uniform samplerCube cubemap; // 0
	
	in vec3 uvw;

	layout(location = 0) out vec4 outColor;

	float luma(vec3 color) {
		return dot(color, vec3(0.299, 0.587, 0.114));
	}

	void main() {
		outColor.rgb = textureLod(cubemap, uvw, 0).rgb;
		outColor.a = 0.0; //luma(outColor.rgb);
	}

#endif