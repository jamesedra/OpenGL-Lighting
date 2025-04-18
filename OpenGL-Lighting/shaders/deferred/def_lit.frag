#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

struct Light {
	vec3 Position;
	vec3 Color;
	float Radius;
};

const int NR_LIGHTS = 32;
layout(std140) uniform LightBlock {
	Light lights[NR_LIGHTS];
};

uniform vec3 viewPos;

void main() {
	vec3 FragPos = texture(gPosition, TexCoords).rgb;
	vec3 Normal = texture(gNormal, TexCoords).rgb;
	vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;
	float Specular = texture(gAlbedoSpec, TexCoords).a;

	vec3 lighting = Albedo * 0.3; // hard-coded ambient component
	vec3 viewDir = normalize(viewPos - FragPos);

	for (int i = 0; i < NR_LIGHTS; i++) {
		// diffuse
		vec3 lightDir = normalize(lights[i].Position - FragPos);
		vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Albedo * lights[i].Color;
		lighting += diffuse;
	}

	FragColor = vec4(lighting, 1.0);
}