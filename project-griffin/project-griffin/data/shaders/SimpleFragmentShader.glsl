#version 440 core

in vec3 color;
in vec2 uv;

out vec3 outColor;

uniform sampler2D tex;

void main(){
    outColor = color;//(texture(tex, uv).rgb * color);
}