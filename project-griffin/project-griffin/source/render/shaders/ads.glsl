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

	out vec4 positionWorldspace; // temp
	out vec3 normalWorldspace;   // temp

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
		
		positionWorldspace = gl_Position; // temp
		normalWorldspace = normalize(modelToWorld * vec4(vertexNormal, 0.0)).xyz;
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
	// temp
	const vec4 lightPosition = { 80.0, 40.0, -40.0, 1.0 };
	//const vec3 lightLa = { 0.6, 0.7, 0.8 };
	const vec3 lightLa = { 0.1, 0.2, 0.3 };
	const vec3 lightLd = { 1.0, 0.5, 0.5 };
	const vec3 lightLs = { 1.0, 0.5, 0.5 };

	/*struct MaterialInfo {
		vec3 Ka; // Ambient reflectivity
		vec3 Kd; // Diffuse reflectivity
		vec3 Ks; // Specular reflectivity
		float shininess; // Specular shininess factor
	};
	uniform MaterialInfo material;*/
	// temp
	uniform vec3 materialKa;
	uniform vec3 materialKd;
	uniform vec3 materialKs;
	//uniform float materialShininess;
	const float materialShininess = 50.0; // temp

	vec3 materialKa = { 1.0, 1.0, 1.0 }; // temp
	
	vec4 cameraPosition = { 120.0, 40.0, 0.0, 1.0 }; // temp
	float lightDistanceSquared = 100000.0; // falloff distance of light squared, temp

	//uniform vec3 diffuseColor;
	//uniform sampler2D diffuse;

	// Input Variables

	in vec4 positionViewspace;
	in vec3 normalViewspace;

	in vec4 positionWorldspace; // temp
	in vec3 normalWorldspace;   // temp

	//in vec4 color;
	//in vec2 uv;
	
	// Output Variables

	layout(location = 0) out vec4 outColor;
	
	// Functions

	vec3 phongModel(vec4 positionViewspace, vec3 normalViewspace)
	{
		vec3 s = normalize(vec3(lightPosition - positionViewspace));
		vec3 v = normalize(-positionViewspace.xyz);
		vec3 r = reflect(-s, normalViewspace);
		vec3 ambient = lightLa * materialKa;
		float sDotN = clamp(dot(s, normalViewspace), 0.0, 1.0);
		vec3 diffuse = lightLd * materialKd * sDotN;
		
		//vec3 halfAngle = normalize(v - s);

		vec3 spec = vec3(0.0);
		if (sDotN > 0.0) {
			spec = lightLs * materialKs * pow(clamp(dot(r,v), 0.0, 1.0), materialShininess);
		}
		return ambient + (diffuse * 0.6) + (spec * 0.5);
	}

	vec3 blinnPhongModel(vec4 positionViewspace, vec3 normalViewspace)
	{
vec3 normal = normalize(normalInterp);
vec3 lightDir = normalize(lightPos - vertPos);

float lambertian = max(dot(lightDir,normal), 0.0);
float specular = 0.0;

if(lambertian > 0.0) {

	vec3 viewDir = normalize(-vertPos);

	// this is blinn phong
	vec3 halfDir = normalize(lightDir + viewDir);
	float specAngle = max(dot(halfDir, normal), 0.0);
	specular = pow(specAngle, 16.0);
 
	// this is phong (for comparison)
	if(mode == 2) {
		vec3 reflectDir = reflect(-lightDir, normal);
		specAngle = max(dot(reflectDir, viewDir), 0.0);
		// note that the exponent is different here
		specular = pow(specAngle, 4.0);
	}
}

gl_FragColor = vec4(ambientColor +
                    lambertian * diffuseColor +
                    specular * specColor, 1.0);

		vec3 s = normalize(vec3(lightPosition - positionViewspace));
		vec3 v = normalize(-positionViewspace.xyz);
		//vec3 r = reflect(-s, normalViewspace);
		vec3 ambient = lightLa * materialKa;
		float sDotN = clamp(dot(s, normalViewspace), 0.0, 1.0);
		vec3 diffuse = lightLd * materialKd * sDotN;
		
		//vec3 halfAngle = normalize(v - s);

		vec3 spec = vec3(0.0);
		if (sDotN > 0.0) {
			spec = lightLs * materialKs * pow(clamp(dot(r,v), 0.0, 1.0), materialShininess);
		}
		return ambient + (diffuse * 0.6) + (spec * 0.5);
	}

	vec3 phongWithFalloff(vec4 positionWorldspace, vec3 normalWorldspace)
	{
		// Phong relfection is ambient + light-diffuse + spec highlights.
		// I = Ia*ka*Oda + fatt*Ip[kd*Od(N.L) + ks(R.V)^n]
		// Ref: http://www.whisqu.se/per/docs/graphics8.htm
		// and http://en.wikipedia.org/wiki/Phong_shading
		// Get light direction for this fragment
		vec3 lightDir = normalize(positionWorldspace - lightPosition).xyz;
		
		float diffuseLighting = clamp(dot(normalWorldspace, -lightDir), 0.0, 1.0); // per pixel diffuse lighting
 
		// Introduce fall-off of light intensity
		diffuseLighting *= (lightDistanceSquared / dot(lightPosition - positionWorldspace, lightPosition - positionWorldspace));
 
		// Using Blinn half angle modification for performance over correctness
		vec3 h = normalize(normalize(cameraPosition - positionWorldspace).xyz - lightDir);
 
		float specLighting = pow(clamp(dot(h, normalWorldspace), 0.0, 1.0), materialShininess);
 
		return vec3(clamp((lightLa * materialKa) +
						  (materialKd * lightLd * diffuseLighting * 0.6) + // Use light diffuse vector as intensity multiplier
						  (materialKs * lightLs * specLighting * 0.5)      // Use light specular vector as intensity multiplier
						  , 0.0, 1.0));
	}

	void main() {
		// Evaluate the lighting equation
		vec3 lightIntensity = phongModel(positionViewspace, normalViewspace);
		//vec3 lightIntensity = phongWithFalloff(positionWorldspace, normalWorldspace);

		//outColor = (texture(diffuse, uv).rgb * color.rgb);
		outColor = vec4(lightIntensity, 1.0);
		//normalReflectance.rgb = (normal + 1.0) * 0.5;
	}

#endif