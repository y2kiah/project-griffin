#version 440 core

in vec3 color;
in vec2 uv;

out vec3 outColor;

uniform sampler2D diffuse;

void main() {
    outColor = (texture(diffuse, uv).rgb * color);
}