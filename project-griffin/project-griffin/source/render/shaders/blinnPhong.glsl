vec3 blinnPhongDirectionalLight(vec4 position, vec3 normal, vec3 lightDirection, vec3 surfaceColor)
{
	vec3 toLight = -normalize(lightDirection);

	vec3 specular = vec3(0.0);
		
	normal = normalize(normal);
	float lambertian = dot(toLight,normal);

	if (lambertian > -0.00001) {
		vec3 viewDir = normalize(vec3(-position));
		vec3 halfDir = normalize(toLight + viewDir);

		float specAngle = max(dot(halfDir, normal), 0.0);

		// determines the specular highlight color with a "metallic" property
		// specular highlight of plastics is light * specular reflectivity, metallic is mostly surface * specular reflectivity
		vec3 specColor = mix(material.Ms * light.Lds,
								material.Ms * surfaceColor,
								material.metallic);

		specular = specColor * vec3(pow(specAngle, material.shininess * 4.0));
		specular *= smoothstep(0.0, 0.2, lambertian); // take out the hard specular edge without losing too much brightness with smoothstep
	}

	vec3 ambient = light.La * surfaceColor * material.Ma;
	vec3 emissive = material.Me;
	vec3 diffuse = light.Lds * surfaceColor * material.Md * max(lambertian, 0.0);

	return ambient + emissive + diffuse + specular;
}

vec3 blinnPhongPointLight(vec4 positionViewspace, vec3 normalViewspace, vec3 surfaceColor)
{
	vec4 positionToLight = light.positionViewspace - positionViewspace;
	float distanceToLight = length(positionToLight);
	vec3 toLight = normalize(vec3(positionToLight));
		
	float attenuation = 1.0 / (light.Kc + light.Kl * distanceToLight + light.Kq * distanceToLight * distanceToLight);

	vec3 normal = normalize(normalViewspace);
	float lambertian = dot(toLight,normal) * attenuation;

	vec3 specular = vec3(0.0);

	if (lambertian > -0.00001) {
		vec3 viewDir = normalize(vec3(-positionViewspace));
		vec3 halfDir = normalize(toLight + viewDir);

		float specAngle = max(dot(halfDir, normal), 0.0);

		// determines the specular highlight color with a "metallic" property
		// specular highlight of plastics is light * specular reflectivity, metallic is mostly surface * specular reflectivity
		vec3 specColor = mix(material.Ms * light.Lds,
								material.Ms * surfaceColor,
								material.metallic);

		specular = specColor * pow(specAngle, material.shininess * 4.0) * attenuation;
		specular *= smoothstep(0.0, 0.2, lambertian); // take out the hard specular edge without losing too much brightness with smoothstep
	}

	vec3 ambient = light.La * surfaceColor * material.Ma;
	vec3 emissive = material.Me;
	vec3 diffuse = light.Lds * surfaceColor * material.Md * max(lambertian, 0.0);
		
	return ambient + emissive + diffuse + specular;
}

vec3 blinnPhongSpotlight(vec4 positionViewspace, vec3 normalViewspace, vec3 surfaceColor)
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
		lambertian = dot(toLight,normal) * angleFalloff * attenuation;

		if (lambertian > -0.00001) {
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
	vec3 diffuse = light.Lds * surfaceColor * material.Md * max(lambertian, 0.0);

	return ambient + emissive + diffuse + specular;
}
