#include "source/render/shaders/layout.glsli"

#ifdef _VERTEX_
	// Uniform Variables

	uniform mat4 modelToWorld;
	uniform mat4 modelView;
	uniform mat4 viewProjection;
	uniform mat4 modelViewProjection;
	uniform mat4 normalMatrix;
	
	// Input Variables

	layout(location = VertexLayout_Position) in vec3 vertexPosition_modelspace;
	layout(location = VertexLayout_Normal) in vec3 vertexNormal;
	layout(location = VertexLayout_Colors) in vec4 vertexColor;
	layout(location = VertexLayout_TextureCoords) in vec2 vertexUV;

	// Output Variables

	out vec4 positionViewspace;
	out vec3 normalViewspace;

	//out vec4 color;
	//out vec2 uv;

	// Functions

	void main()
	{
		// Get the position and normal in viewspace
		normalViewspace = normalize(normalMatrix * vec4(vertexNormal, 0.0)).xyz;
		positionViewspace = modelView * vec4(vertexPosition_modelspace, 1.0);
		
		//color = vertexColor;
		//uv = vertexUV;

		gl_Position = modelViewProjection * vec4(vertexPosition_modelspace, 1.0);
	}
	
#endif

#ifdef _FRAGMENT_
	// Uniform Variables

	/*struct LightInfo {
		vec4 position; // Light position in eye coords.
		vec3 La; // Ambient light intensity
		vec3 Ld; // Diffuse light intensity
		vec3 Ls; // Specular light intensity
	};
	uniform LightInfo light;*/
	const vec3 lightLa = { 0.6, 0.7, 0.8 };
	//const vec3 lightLa = { 0.1, 0.2, 0.3 };
	const vec3 lightLd = { 1.0, 0.2, 0.2 };
	const vec3 lightLs = lightLd;

	/*struct MaterialInfo {
		vec3 Ka; // Ambient reflectivity
		vec3 Kd; // Diffuse reflectivity
		vec3 Ks; // Specular reflectivity
		float shininess; // Specular shininess factor
	};
	uniform MaterialInfo material;*/
	// temp
	//uniform vec3 materialKa;
	uniform vec3 materialKd;
	uniform vec3 materialKs;
	//uniform float materialShininess;
	const float materialShininess = 30.0; // temp
	const float materialMetallic = 0.85; // temp

	vec3 materialKa = materialKd; // temp

	const vec4 lightPosition = { 80.0, 0.0, 0.0, 1.0 }; // temp
	float lightDistanceSquared = 20000.0; // falloff distance of light squared, temp

	//uniform vec3 diffuseColor;
	//uniform sampler2D diffuse;

	// Input Variables

	in vec4 positionViewspace;
	in vec3 normalViewspace;

	//in vec4 color;
	//in vec2 uv;
	
	// Output Variables

	layout(location = 0) out vec4 outColor;
	
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
		vec3 diffuse = lightLd * materialKd * lambertian;

		//return ambient + (diffuse * 0.6) + (specular * 0.5);
		return ambient + diffuse + specular;
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
		vec3 diffuse = lightLd * materialKd * lambertian;

		//return ambient + (diffuse * 0.6) + (specular * 0.5);
		return ambient + diffuse + specular;
		//return max(diffuse + specular, ambient);
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
		vec3 diffuse = lightLd * materialKd * lambertian;
		
		//return ambient + (diffuse * 0.6) + (specular * 0.5);
		return ambient + diffuse + specular;
		//return max(diffuse + specular, ambient);
	}

	vec3 blinnPhongSpotlight(vec4 positionViewspace, vec3 normalViewspace)
	{
		vec4 positionToLight = lightPosition - positionViewspace;
		vec3 lightDir = normalize(vec3(positionToLight));

		// temp spotlight stuff
		vec4 spotlightDirection = { -1.0, 0.0, -1.0, 0.0 };
		float spotlightCutoff = 0.98;
		float spotlightEdgeFalloff = (1.0 - spotlightCutoff) * 0.2;
		vec3 sd = normalize(vec3(-spotlightDirection));
		/////

		float lambertian = 0.0;
		vec3 specular = vec3(0.0);

		float lightAngle = max(dot(sd,lightDir), 0.0);
		if (lightAngle > spotlightCutoff) {
			float angleFalloff = smoothstep(spotlightCutoff, spotlightCutoff + spotlightEdgeFalloff, lightAngle);
			float distanceFalloff = (lightDistanceSquared / dot(positionToLight, positionToLight));

			vec3 normal = normalize(normalViewspace);
			lambertian = max(dot(lightDir,normal), 0.0) * angleFalloff * distanceFalloff;

			if (lambertian > 0.0) {
				vec3 viewDir = normalize(vec3(-positionViewspace));
				vec3 halfDir = normalize(lightDir + viewDir);

				float specAngle = max(dot(halfDir, normal), 0.0);

				// determines the specular highlight color with a "metallic" property
				// specular highlight of plastics is light combined with surface, metalic is mostly surface
				vec3 specColor = mix(lightLs * materialKs, materialKs, materialMetallic);

				specular = specColor * pow(specAngle, materialShininess * 4.0) * angleFalloff * distanceFalloff;
			}
		}

		vec3 ambient = lightLa * materialKa;
		vec3 diffuse = lightLd * materialKd * lambertian;

		//return ambient + (diffuse * 0.6) + (specular * 0.5);
		return ambient + diffuse + specular;
		//return max(diffuse + specular, ambient);
	}

	void main() {
		// Evaluate the lighting equation
		//vec3 lightIntensity = phongPointLight(positionViewspace, normalViewspace);
		
		//vec3 lightIntensity = blinnPhongDirectionalLight(positionViewspace, normalViewspace);
		//vec3 lightIntensity = blinnPhongPointLight(positionViewspace, normalViewspace);
		vec3 lightIntensity = blinnPhongSpotlight(positionViewspace, normalViewspace);

		//outColor = (texture(diffuse, uv).rgb * color.rgb);
		outColor = vec4(lightIntensity, 1.0);
		//normalReflectance.rgb = (normal + 1.0) * 0.5;
	}

#endif