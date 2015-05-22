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
	
	out vec3 lightIntensity;
	
	/*struct LightInfo {
		vec4 position; // Light position in eye coords.
		vec3 La; // Ambient light intensity
		vec3 Ld; // Diffuse light intensity
		vec3 Ls; // Specular light intensity
	};
	uniform LightInfo light;*/
	// temp
	const vec4 lightPosition = { 0.0, 40.0, -40.0, 1.0 };
	const vec3 lightLa = { 0.4, 0.6, 0.8 };
	const vec3 lightLd = { 1.0, 1.0, 1.0 };
	const vec3 lightLs = { 1.0, 1.0, 1.0 };

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
	uniform float materialShininess;

	const vec3 materialKa = materialKd; // temp

	uniform mat4 modelToWorld;
	uniform mat4 modelView;
	uniform mat4 viewProjection;
	uniform mat4 modelViewProjection;
	uniform mat4 normalMatrix;
	
	layout(location = VertexLayout_Position) in vec3 vertexPosition_modelspace;
	layout(location = VertexLayout_Normal) in vec3 vertexNormal;
	layout(location = VertexLayout_Colors) in vec4 vertexColor;
	layout(location = VertexLayout_TextureCoords) in vec2 vertexUV;

	out vec3 normal;
	out vec4 color;
	out vec2 uv;

	void getEyeSpace(out vec3 normalViewspace, out vec4 positionViewspace)
	{
		normalViewspace = normalize(normalMatrix * vec4(vertexNormal, 0.0)).xyz;
		positionViewspace = modelView * vec4(vertexPosition_modelspace, 1.0);
	}
	
	vec3 phongModel(vec4 position, vec3 norm)
	{
		vec3 s = normalize(vec3(lightPosition - position));
		vec3 v = normalize(-position.xyz);
		vec3 r = reflect(-s, norm);
		vec3 ambient = lightLa * materialKa;
		float sDotN = max(dot(s,norm), 0.0);
		vec3 diffuse = lightLd * materialKd * sDotN;
		vec3 spec = vec3(0.0);
		if (sDotN > 0.0) {
			spec = lightLs * materialKs * pow(max(dot(r,v), 0.0), materialShininess);
		}
		return ambient + diffuse + spec;
	}
	
	void main()
	{
		vec4 positionViewspace;
		// Get the position and normal in viewspace
		getEyeSpace(normal, positionViewspace);

		// Evaluate the lighting equation
		lightIntensity = phongModel(positionViewspace, normal);
		
		color = vertexColor;
		uv = vertexUV;

		gl_Position = modelViewProjection * vec4(vertexPosition_modelspace, 1.0);
	}
	
#endif

#ifdef _FRAGMENT_
	
	//uniform vec3 diffuseColor;
	//uniform sampler2D diffuse;

	in vec3 normal;
	in vec4 color;
	in vec2 uv;

	in vec3 lightIntensity;
	
	layout(location = 0) out vec4 outColor;
	
	void main() {
		//outColor = (texture(diffuse, uv).rgb * color.rgb);
		outColor = vec4(lightIntensity, 1.0);
		//normalReflectance.rgb = (normal + 1.0) * 0.5;
	}

#endif