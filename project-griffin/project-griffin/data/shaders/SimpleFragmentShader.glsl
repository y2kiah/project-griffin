#version 440 core

in vec2 texCoord;

out vec4 outColor;

uniform sampler2D tex;

void main(){
    outColor = texture(tex, texCoord);
}