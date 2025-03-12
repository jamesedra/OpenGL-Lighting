#version 330 core

layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoords;

void main() {
	vec4 fragPos = projection * view * model * vec4(aPos, 1.0);
	gl_Position = fragPos;

	// get screen space uvs
	vec3 ndc = fragPos.xyz / fragPos.w;
	TexCoords = ndc.xy * 0.5 + 0.5;
}