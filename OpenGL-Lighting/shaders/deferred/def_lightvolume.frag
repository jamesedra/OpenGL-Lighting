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
// uniform Light lights[NR_LIGHTS];
layout(std140) uniform LightBlock {
	Light lights[NR_LIGHTS];
};

uniform vec3 viewPos;
uniform int lightIndex;

void main() {
	vec3 FragPos = texture(gPosition, TexCoords).rgb;
	vec3 Normal = texture(gNormal, TexCoords).rgb;
	vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;
	float Specular = texture(gAlbedoSpec, TexCoords).a;

	vec3 viewDir = normalize(viewPos - FragPos);

	Light currLight = lights[lightIndex];
	float distance = length(currLight.Position - FragPos);
	if (distance > currLight.Radius) discard;

	float attenuation = 1.0 - (distance / currLight.Radius);
    attenuation *= attenuation;

	vec3 lightDir = normalize(currLight.Position - FragPos);

	float diff = max(dot(Normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, Normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);

    vec3 lighting = (Albedo * currLight.Color * diff + currLight.Color * Specular) * attenuation;

    FragColor = vec4(lighting, 1.0);
}