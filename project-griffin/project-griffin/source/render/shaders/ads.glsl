#include "source/render/shaders/layout.glsli"

#ifdef _VERTEX_
	// Uniform Variables

	uniform mat4 modelToWorld;
	uniform mat4 modelView;
	uniform mat4 viewProjection;
	uniform mat4 modelViewProjection;
	uniform mat4 normalMatrix;

	uniform float frustumNear;
	uniform float frustumFar;
	uniform float inverseFrustumDistance; // inverse max distance of the camera, to get linear depth

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

	/*struct LightInfo {
		vec4 positionViewspace; // Light position in eye coords.
		vec3 La; // Ambient light intensity
		vec3 Ld; // Diffuse light intensity
		vec3 Ls; // Specular light intensity
	};
	uniform LightInfo light;*/
	const vec3 lightLa = { 0.6, 0.7, 0.8 };
	//const vec3 lightLa = { 0.1, 0.2, 0.3 };
	const vec3 lightLd = { 0.8, 0.6, 0.3 };
	const vec3 lightLs = lightLd;

	/*struct MaterialInfo {
		vec3 Ka; // Ambient reflectivity
		vec3 Kd; // Diffuse reflectivity
		vec3 Ks; // Specular reflectivity
		float shininess; // Specular shininess factor
		float metallic;
	};
	uniform MaterialInfo material;*/
	// temp
	//uniform vec3 materialKa; // ambient color
	uniform vec3 materialKe; // emissive color
	uniform vec3 materialKd; // diffuse color
	uniform vec3 materialKs; // specular color
	//uniform float materialShininess;
	const float materialShininess = 30.0; // temp
	const float materialMetallic = 0.85; // temp

	vec3 materialKa = materialKd; // temp

	const vec4 lightPosition = { 0.0, 0.0, 0.0, 1.0 }; // temp
	float lightDistanceSquared = 5000.0; // falloff distance of light squared, temp

	//uniform vec3 diffuseColor;
	uniform sampler2D diffuseMap;	// 4

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
	layout(location = 1) out vec3 eyeSpacePosition;
	layout(location = 2) out vec4 normalReflectance;
	
	// Functions

	vec3 phongPointLight(vec4 positionViewspace, vec3 normalViewspace)
	{
		vec3 normal = normalize(normalViewspace);
		vec3 lightDir = normalize(vec3(lightPosition - positionViewspace));
		
		float lambertian = max(dot(lightDir, normal), 0.0);

		vec3 specular = vec3(0.0);
		if (lambertian > 0.0) {
			vec3 viewDir = normalize(vec3(-positionViewspace));
			vec3 reflectDir = normalize(reflect(-lightDir, normalViewspace));

			float specAngle = max(dot(reflectDir,viewDir), 0.0);
			specular = lightLs * materialKs * pow(specAngle, materialShininess);
		}

		vec3 ambient = lightLa * materialKa;
		vec3 emissive = materialKe;
		vec3 diffuse = lightLd * materialKd * lambertian;

		return ambient + emissive + diffuse + specular;
	}

	vec3 blinnPhongDirectionalLight(vec4 positionViewspace, vec3 normalViewspace)
	{
		vec3 normal = normalize(normalViewspace);
		vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0)); // temp

		vec3 specular = vec3(0.0);
		
		float lambertian = max(dot(lightDir,normal), 0.0);

		if (lambertian > 0.0) {
			vec3 viewDir = normalize(vec3(-positionViewspace));
			vec3 halfDir = normalize(lightDir + viewDir);

			float specAngle = max(dot(halfDir, normal), 0.0);
			specular = lightLs * materialKs * pow(specAngle, materialShininess * 4.0);
		}

		vec3 ambient = lightLa * materialKa;
		vec3 emissive = materialKe;
		vec3 diffuse = lightLd * materialKd * lambertian;

		return ambient + emissive + diffuse + specular;
	}

	vec3 blinnPhongPointLight(vec4 positionViewspace, vec3 normalViewspace)
	{
		vec4 positionToLight = lightPosition - positionViewspace;
		vec3 lightDir = normalize(vec3(positionToLight));

		vec3 specular = vec3(0.0);
		
		float distanceFalloff = (lightDistanceSquared / dot(positionToLight, positionToLight));

		vec3 normal = normalize(normalViewspace);
		float lambertian = max(dot(lightDir,normal), 0.0) * distanceFalloff;

		if (lambertian > 0.0) {
			vec3 viewDir = normalize(vec3(-positionViewspace));
			vec3 halfDir = normalize(lightDir + viewDir);

			float specAngle = max(dot(halfDir, normal), 0.0);
			specular = lightLs * materialKs * pow(specAngle, materialShininess * 4.0) * distanceFalloff;
		}

		vec3 ambient = lightLa * materialKa;
		vec3 emissive = materialKe;
		vec3 diffuse = lightLd * materialKd * lambertian;
		
		return ambient + emissive + diffuse + specular;
	}

	vec3 blinnPhongSpotlight(vec4 positionViewspace, vec3 normalViewspace)
	{
		vec4 positionToLight = lightPosition - positionViewspace;
		vec3 lightDir = normalize(vec3(positionToLight));

		// temp spotlight stuff
		vec4 spotlightDirection = { 0.0, 0.0, -1.0, 0.0 };
		float spotlightCutoff = 0.96;
		float spotlightEdgeFalloff = (1.0 - spotlightCutoff) * 0.4;
		vec3 sd = normalize(vec3(-spotlightDirection));
		float spotlightPower = 1.5; // does this make sense?? used to clamp the distance falloff
		/////

		// Diffuse surface color
		#ifdef _HAS_DIFFUSE_MAP
			vec3 surfaceColor = texture(diffuseMap, uv).rgb;
		#else
			vec3 surfaceColor = materialKd;
		#endif
		/////

		float lambertian = 0.0;
		vec3 specular = vec3(0.0);

		float lightAngle = max(dot(sd,lightDir), 0.0);
		if (lightAngle > spotlightCutoff) {
			float angleFalloff = smoothstep(spotlightCutoff, spotlightCutoff + spotlightEdgeFalloff, lightAngle);
			float distanceFalloff = clamp((lightDistanceSquared / dot(positionToLight, positionToLight)), 0.0, spotlightPower);

			vec3 normal = normalize(normalViewspace);
			lambertian = max(dot(lightDir,normal), 0.0) * angleFalloff * distanceFalloff;

			if (lambertian > 0.0) {
				vec3 viewDir = normalize(vec3(-positionViewspace));
				vec3 halfDir = normalize(lightDir + viewDir);

				float specAngle = max(dot(halfDir, normal), 0.0);

				// determines the specular highlight color with a "metallic" property
				// specular highlight of plastics is light * specular reflectivity, metallic is mostly surface * specular reflectivity
				vec3 specColor = mix(lightLs * materialKs,
									 materialKs * surfaceColor,
									 materialMetallic);

				specular = specColor * pow(specAngle, materialShininess * 4.0) * angleFalloff * distanceFalloff;
			}
		}

		vec3 ambient = lightLa * surfaceColor * materialKa;
		vec3 emissive = materialKe;

		vec3 diffuse = lightLd * surfaceColor * materialKd * lambertian;

		return ambient + emissive + diffuse + specular;
		//return texture(diffuseMap, uv).rgb;
		//return vec3(uv.x, uv.y, 0.0);
		//return normalViewspace;
	}

	float luma(vec3 color) {
		return dot(color, vec3(0.299, 0.587, 0.114));
	}

	void main() {
		// Evaluate the lighting equation
		//vec3 lightIntensity = phongPointLight(positionViewspace, normalViewspace);
		
		//vec3 lightIntensity = blinnPhongDirectionalLight(positionViewspace, normalViewspace);
		//vec3 lightIntensity = blinnPhongPointLight(positionViewspace, normalViewspace);
		vec3 lightIntensity = blinnPhongSpotlight(positionViewspace, normalViewspace);

		//outColor = (texture(diffuse, uv).rgb * color.rgb);
		albedoDisplacement = vec4(lightIntensity, luma(lightIntensity));
		eyeSpacePosition = positionViewspace.xyz;
		normalReflectance = vec4(normalViewspace.xyz, 0.0);// vec4((normalViewspace + 1.0) * 0.5, 0.0);
		gl_FragDepth = linearDepth;
	}

#endif