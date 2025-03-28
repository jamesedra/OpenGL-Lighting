#version 330 core

in VS_OUT {
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoords;
	vec4 FragPosLightSpace;
	vec3 TangentLightPos;
	vec3 TangentViewPos;
	vec3 TangentFragPos;
	mat3 TBN;
	mat3 nonTransTBN;
} fs_in;

out vec4 FragColor;

uniform sampler2D normalMap;

void main() {
	vec3 n = texture(normalMap, fs_in.TexCoords).rgb;
	n = normalize(n * 2.0 - 1.0);
	n = normalize(fs_in.nonTransTBN * n);

	FragColor = vec4(n, 1.0);
}