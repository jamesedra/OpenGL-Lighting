#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D ssao;

struct Light {
vec3 Position;
vec3 Color;

float Linear;
float Quadratic;
float Radius;
};
uniform Light light;

void main() {
	vec3 FragPos = texture(gPosition, TexCoords).rgb;
	vec3 Normal = texture(gNormal, TexCoords).rgb;
	vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
	float Specular = texture(gAlbedoSpec, TexCoords).a;

	float AmbientOcclusion = texture(ssao, TexCoords).r;

	// blinn-phong view space
	vec3 ambient = vec3(0.3 * Diffuse * AmbientOcclusion);
	vec3 lighting = ambient;
	vec3 viewDir = normalize(-FragPos);
	vec3 lightDir = normalize(light.Position - FragPos);
	vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * light.Color;

	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(Normal, halfwayDir), 0.0), 8.0);
	vec3 specular = light.Color * spec;
	float dist = length(light.Position - FragPos);
	float attenuation = 1.0 / (1.0 + light.Linear * dist + light.Quadratic * dist * dist);
	diffuse *= attenuation;
	specular *= attenuation;
	lighting += diffuse + specular;

	FragColor = vec4(lighting, 1.0);
}

