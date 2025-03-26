#version 330 core
out vec4 FragColor;

in vec3 localPos;

uniform samplerCube environmentMap;

void main() {
	// vec3 envColor = texture(environmentMap, normalize(localPos)).rgb;

	// prefilter map testing
	vec3 envColor = textureLod(environmentMap, normalize(localPos), 2.5).rgb;

	envColor = envColor / (envColor + vec3(1.0));
	envColor = pow(envColor, vec3(1.0/2.2));

	FragColor = vec4(envColor, 1.0);
}