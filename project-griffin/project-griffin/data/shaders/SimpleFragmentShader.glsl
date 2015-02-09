#version 440 core

in vec3 normal;
in vec4 color;
in vec2 uv;

out vec3 outColor;

uniform sampler2D diffuse;

void main() {
    //outColor = (texture(diffuse, uv).rgb * color.rgb);
	//outColor = color.rgb;
	//outColor = normal;
	outColor = vec3(1.0, 0, 0);
}