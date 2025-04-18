#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D hdrBuffer;
uniform float exposure;

void main() {
	const float gamma = 2.2;
	vec3 hdrColor = texture(hdrBuffer, TexCoords).rgb;

	// reinhard tonemapping
	vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
	mapped = vec3(1.0) - exp(-hdrColor * exposure);
	mapped = pow(mapped, vec3(1.0 / gamma));
	//FragColor = vec4(mapped, 1.0);

	vec2 testOut = texture(hdrBuffer, TexCoords).rg;
	FragColor = vec4(testOut, 0.0, 1.0);
}