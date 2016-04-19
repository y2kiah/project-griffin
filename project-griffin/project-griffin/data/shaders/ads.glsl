//#define UniformLayout_ModelToWorld        0
//#define UniformLayout_ViewProjection      1
//#define UniformLayout_ModelViewProjection 2

#define VertexLayout_Position      0
#define VertexLayout_Normal        1
#define VertexLayout_Tangent       2
#define VertexLayout_Bitangent     3
#define VertexLayout_TextureCoords 4   // consumes up to 8 locations
#define VertexLayout_Colors        12  // consumes up to 8 locations
#define VertexLayout_CustomStart   20  // use for first custom binding location and increment

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
	uniform Light light;

	struct Material {
		vec3 Ma;					// Ambient reflectivity
		vec3 Md;					// Diffuse reflectivity
		vec3 Ms;					// Specular reflectivity
		vec3 Me;					// Emissive reflectivity
		float shininess;			// Specular shininess factor
		float metallic;				// Metallic determines color of specular highlight
	};
	uniform Material material;

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
	
	// Globals
	vec3 surfaceColor;

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
	
	// Functions

	vec3 blinnPhongDirectionalLight(vec4 positionViewspace, vec3 normalViewspace)
	{
		vec3 toLight = normalize(vec3(light.positionViewspace));

		vec3 specular = vec3(0.0);
		
		vec3 normal = normalize(normalViewspace);
		float lambertian = max(dot(toLight,normal), 0.0);

		if (lambertian > 0.0) {
			vec3 viewDir = normalize(vec3(-positionViewspace));
			vec3 halfDir = normalize(toLight + viewDir);

			float specAngle = max(dot(halfDir, normal), 0.0);

			// determines the specular highlight color with a "metallic" property
			// specular highlight of plastics is light * specular reflectivity, metallic is mostly surface * specular reflectivity
			vec3 specColor = mix(material.Ms * light.Lds,
								 material.Ms * surfaceColor,
								 material.metallic);

			specular = specColor * pow(specAngle, material.shininess * 4.0);
		}

		vec3 ambient = light.La * surfaceColor * material.Ma;
		vec3 emissive = material.Me;
		vec3 diffuse = light.Lds * surfaceColor * material.Md * lambertian;

		return ambient + emissive + diffuse + specular;
	}

	vec3 blinnPhongPointLight(vec4 positionViewspace, vec3 normalViewspace)
	{
		vec4 positionToLight = light.positionViewspace - positionViewspace;
		float distanceToLight = length(positionToLight);
		vec3 toLight = normalize(vec3(positionToLight));
		
		float attenuation = 1.0 / (light.Kc + light.Kl * distanceToLight + light.Kq * distanceToLight * distanceToLight);

		vec3 normal = normalize(normalViewspace);
		float lambertian = max(dot(toLight,normal), 0.0) * attenuation;

		vec3 specular = vec3(0.0);

		if (lambertian > 0.0) {
			vec3 viewDir = normalize(vec3(-positionViewspace));
			vec3 halfDir = normalize(toLight + viewDir);

			float specAngle = max(dot(halfDir, normal), 0.0);

			// determines the specular highlight color with a "metallic" property
			// specular highlight of plastics is light * specular reflectivity, metallic is mostly surface * specular reflectivity
			vec3 specColor = mix(material.Ms * light.Lds,
								 material.Ms * surfaceColor,
								 material.metallic);

			specular = specColor * pow(specAngle, material.shininess * 4.0) * attenuation;
		}

		vec3 ambient = light.La * material.Ma;
		vec3 emissive = material.Me;
		vec3 diffuse = light.Lds * material.Md * lambertian;
		
		return ambient + emissive + diffuse + specular;
	}

	vec3 blinnPhongSpotlight(vec4 positionViewspace, vec3 normalViewspace)
	{
		vec4 positionToLight = light.positionViewspace - positionViewspace;
		float distanceToLight = length(positionToLight);
		vec3 toLight = normalize(vec3(positionToLight));

		float spotlightEdgeFalloff = (1.0 - light.spotAngleCutoff) * light.spotEdgeBlendPct;

		float lambertian = 0.0;
		vec3 specular = vec3(0.0);

		float lightAngle = max(-dot(normalize(light.directionViewspace), toLight), 0.0);

		if (lightAngle > light.spotAngleCutoff) {
			float angleFalloff = smoothstep(light.spotAngleCutoff, light.spotAngleCutoff + spotlightEdgeFalloff, lightAngle);
			
			float attenuation = 1.0 / (light.Kc + light.Kl * distanceToLight + light.Kq * distanceToLight * distanceToLight);

			vec3 normal = normalize(normalViewspace);
			lambertian = max(dot(toLight,normal), 0.0) * angleFalloff * attenuation;

			if (lambertian > 0.0) {
				vec3 viewDir = normalize(vec3(-positionViewspace));
				vec3 halfDir = normalize(toLight + viewDir);

				float specAngle = max(dot(halfDir, normal), 0.0);

				// determines the specular highlight color with a "metallic" property
				// specular highlight of plastics is light * specular reflectivity, metallic is mostly surface * specular reflectivity
				vec3 specColor = mix(material.Ms * light.Lds,
									 material.Ms * surfaceColor,
									 material.metallic);

				specular = specColor * pow(specAngle, material.shininess * 4.0) * angleFalloff * attenuation;
			}
		}

		vec3 ambient = light.La * surfaceColor * material.Ma;
		vec3 emissive = material.Me;
		vec3 diffuse = light.Lds * surfaceColor * material.Md * lambertian;

		return ambient + emissive + diffuse + specular;
	}

	float luma(vec3 color) {
		return dot(color, vec3(0.299, 0.587, 0.114));
	}

	void main() {
		// get diffuse surface color
		//#ifdef _HAS_DIFFUSE_MAP
		//	surfaceColor = texture(diffuseMap, uv).rgb;
		//#else
			surfaceColor = material.Md;
		//#endif
		/////

		//vec3 lightIntensity = blinnPhongDirectionalLight(positionViewspace, normalViewspace);
		//vec3 lightIntensity = blinnPhongPointLight(positionViewspace, normalViewspace);
		vec3 lightIntensity = blinnPhongSpotlight(positionViewspace, normalViewspace);

		albedoDisplacement = vec4(lightIntensity, 1.0); //vec4(surfaceColor, 0.0);
		eyeSpacePosition = vec4(positionViewspace.xyz, 0.0);
		normalReflectance = vec4(normalViewspace, 0.0);
		gl_FragDepth = linearDepth;
	}

#endif