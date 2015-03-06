#include "source/render/shaders/layout.glsli"

#ifdef _VERTEX_
	
	uniform mat4 modelView;
	uniform mat4 projection;
	
	uniform float frustumNear;
	uniform float frustumFar;

	layout(location = VertexLayout_Position) in vec3 vertexPosition_modelspace;
	layout(location = VertexLayout_Normal) in vec3 vertexNormal;

	out float linearDepth;

	void main()
	{
		vec4 viewPos = modelView * vec4(vertexPosition_modelspace, 1.0); // vertex to eyespace
		linearDepth = (-viewPos.z - frustumNear) / (frustumFar - frustumNear); // map near..far to 0..1
		gl_Position = projection * viewPos;

		//normal = normalize(modelToWorld * vec4(vertexNormal, 0.0)).xyz;
		//color = vertexColor;
		//uv = vertexUV;
	}

#endif

#ifdef _FRAGMENT_
	
	in float linearDepth;

	out vec4 outColor;

	void main()
	{
		outColor = vec4(linearDepth, 0.0, 0.0, 1.0);
	}

#endif