#include "source/render/shaders/layout.glsli"

#ifdef _VERTEX_

	uniform mat4 modelToWorld;
	uniform mat4 viewProjection;
	uniform mat4 modelViewProjection;
	uniform mat4 normalMatrix;
	//layout(location = UniformLayout_ModelView) uniform mat4 modelView;
	//layout(location = UniformLayout_Projection) uniform mat4 projection;
	//layout(location = UniformLayout_ModelViewProjection) uniform mat4 modelViewProjection;

	layout(location = VertexLayout_Position) in vec3 vertexPosition_modelspace;
	layout(location = VertexLayout_Normal) in vec3 vertexNormal;
	layout(location = VertexLayout_Colors) in vec4 vertexColor;
	layout(location = VertexLayout_TextureCoords) in vec2 vertexUV;

	out vec3 normal;
	out vec4 color;
	out vec2 uv;

	void main() {
		gl_Position = viewProjection * modelToWorld * vec4(vertexPosition_modelspace, 1.0);

		normal = normalize(modelToWorld * vec4(vertexNormal, 0.0)).xyz;
		color = vertexColor;
		uv = vertexUV;
	}

#endif

#ifdef _FRAGMENT_
	uniform vec3 diffuseColor;

	in vec3 normal;
	in vec4 color;
	in vec2 uv;

	layout(location = 0) out vec4 diffuseDisplacement;
	layout(location = 1) out vec3 eyeSpacePosition;
	layout(location = 2) out vec4 normalReflectance;

	uniform sampler2D diffuse;

	void main() {
		//outColor = (texture(diffuse, uv).rgb * color.rgb);
		diffuseDisplacement.rgb = diffuseColor;
		normalReflectance.rgb = (normal + 1.0) * 0.5;
	}

#endif