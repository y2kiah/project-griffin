#include "source/render/shaders/layout.glsli"
#include "source/render/shaders/ubo.glsli"

#ifdef _VERTEX_
	
	// Input Variables

	layout(location = VertexLayout_Position) in vec3 vertexPosition_modelspace;
	layout(location = VertexLayout_Normal) in vec3 vertexNormal;
	layout(location = VertexLayout_Colors) in vec4 vertexColor;
	layout(location = VertexLayout_Tangent) in vec3 vertexTangent;
	layout(location = VertexLayout_TextureCoords) in vec2 vertexUV;

	// Output Variables

	out vec4 positionViewspace;
	out vec3 normalViewspace;
	out float linearDepth;
	
	//out vec4 color;
	out vec2 uv;
	out vec3 tangent;

	// Functions

	void main()
	{
		// Get the position and normal in viewspace
		normalViewspace = normalize(normalMatrix * vec4(vertexNormal, 0.0)).xyz;
		positionViewspace = modelView * vec4(vertexPosition_modelspace, 1.0);
		
		linearDepth = (-positionViewspace.z - frustumNear) * inverseFrustumDistance; // map near..far linearly to 0..1
		

		//color = vertexColor;
		uv = vertexUV;
		tangent = vertexTangent;
		//z = log(z / zn) / log(zf / zn); // logarithmic z

		gl_Position = modelViewProjection * vec4(vertexPosition_modelspace, 1.0);
	}
	
#endif

#ifdef _FRAGMENT_
	
	// Uniform Variables

	struct Light {
		vec4 positionViewspace;		// Light position in viewspace
		vec3 directionViewspace;	// Light spot direction in viewspace
		vec3 La;					// light color ambient
		vec3 Lds;					// light color diffuse and specular
		float Kc;					// attenuation base constant
		float Kl;					// attenuation linear constant
		float Kq;					// attenuation quadratic constant
		float spotAngleCutoff;		// spotlight angle cutoff, dot product comparison (% from 90deg)
		float spotEdgeBlendPct;		// spotlight edge blend, in % of spot radius
	};

	struct Material {
		vec3 Ma;					// Ambient reflectivity
		vec3 Md;					// Diffuse reflectivity
		vec3 Ms;					// Specular reflectivity
		vec3 Me;					// Emissive reflectivity
		float shininess;			// Specular shininess factor
		float metallic;				// Metallic determines color of specular highlight
	};

	uniform Light light;
	uniform Material material;

	//uniform vec3 diffuseColor;

	// these would be used to support models with > 1 channel
	// these should go in the per-material uniform buffer
	//uniform int diffuseUVChannelIndex0 = 0;
	//uniform int diffuseUVChannelIndex1 = 0;
	//uniform int diffuseUVChannelIndex2 = 0;
	//uniform int diffuseUVChannelIndex3 = 0;
	// etc... for each texture
	//
	//uniform int diffuseVertexColorChannel = 0;
	
	// Input Variables

	in vec4 positionViewspace;
	in vec3 normalViewspace;
	in float linearDepth;

	//in vec4 color;
	in vec2 uv;
	in vec3 tangent;

	// Output Variables

	layout(location = 0) out vec4 albedoDisplacement;
	layout(location = 1) out vec4 eyeSpacePosition;
	layout(location = 2) out vec4 normalReflectance;

	layout(binding = SamplerBinding_Diffuse1) uniform sampler2D diffuse1;
	
	// Subroutines

	subroutine vec3 SurfaceColorSubroutine();
	layout(location = SubroutineUniform_SurfaceColor) subroutine uniform SurfaceColorSubroutine getSurfaceColor;

	// Functions

	#include "source/render/shaders/blinnPhong.glsl"

	layout(index = 0) subroutine(SurfaceColorSubroutine) vec3 getSurfaceColorFromTexture()
	{
		return texture(diffuse1, uv).rgb;
	}

	layout(index = 1) subroutine(SurfaceColorSubroutine) vec3 getSurfaceColorFromMaterial()
	{
		return material.Md;
	}

	float luma(vec3 color) {
		return dot(color, vec3(0.299, 0.587, 0.114));
	}

	void main() {
		vec3 surfaceColor = getSurfaceColor();

		vec3 lightIntensity = blinnPhongDirectionalLight(positionViewspace, normalViewspace, surfaceColor);
		//vec3 lightIntensity = blinnPhongPointLight(positionViewspace, normalViewspace, surfaceColor);
		//vec3 lightIntensity = blinnPhongSpotlight(positionViewspace, normalViewspace, surfaceColor);

		albedoDisplacement = vec4(lightIntensity, 1.0); //vec4(surfaceColor, 0.0);
		eyeSpacePosition = vec4(positionViewspace.xyz, 0.0);
		normalReflectance = vec4(normalViewspace, 0.0);
		gl_FragDepth = linearDepth;
	}

#endif