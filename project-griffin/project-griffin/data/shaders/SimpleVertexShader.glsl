#version 440 core

layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec2 vertexUV;

out vec3 color;
out vec2 uv;

void main() {
	gl_Position.xyz = vertexPosition_modelspace;
	gl_Position.w = 1.0;

	color = vertexColor;
	uv = vertexUV;
}