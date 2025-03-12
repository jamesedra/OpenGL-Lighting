#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gAlbedoSpec;
uniform float ambient = 0.5;

uniform vec3 viewPos;

void main() {
	vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;

	FragColor = vec4(Albedo * ambient, 1.0);
}