#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;

struct Material {
	vec3 albedo;
	float metallic;
	float roughness;
	float ao;
};
uniform Material material;

struct Light {
	vec3 position;
	vec3 color;
};
uniform Light lights[4];


vec3 Fresnel(float cosTheta, vec3 F0);
float NormalDistribution(float nDotH, float roughness);
float GeometryEq(float dotProd, float roughness);

const float PI = 3.14159265359;

void main() {
	vec3 n = normalize(Normal);				
	vec3 v = normalize(viewPos - FragPos);					// view dir

	float roughness = max(material.roughness, 0.0001);

	vec3 Lo = vec3(0.0);									// Irradiance

	for (int i = 0; i < 4; i++) {
		vec3 l = normalize(lights[i].position - FragPos);	// light dir
		vec3 h = normalize(v + l);							// halfway vector

		float distance = length(lights[i].position - FragPos);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = lights[i].color * attenuation;

		// Dot product setup
		float nDotL = max(dot(n, l), 0.0);
		float vDotH = max(dot(v, h), 0.0);
		float nDotH = max(dot(n, h), 0.0);
		float nDotV = max(dot(n, v), 0.0);

		// Specular BRDF
		vec3 F0 = vec3(0.04);
		F0 = mix(F0, material.albedo, material.metallic);
		
		vec3 F = Fresnel(vDotH, F0);
		float D = NormalDistribution(nDotH, roughness);
		float G = GeometryEq(nDotL, roughness) * GeometryEq(nDotV, roughness);

		vec3 SpecBRDF_nom = D * G * F;
		float SpecBRDF_denom = 4.0 * nDotV * nDotL;
		vec3 SpecBRDF = SpecBRDF_nom / max(SpecBRDF_denom, 0.001);

		// Diffuse BRDF
		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - material.metallic;		// decreases diffuse reflections as metallic value increases.
		
		vec3 fLambert = material.albedo;
		vec3 DiffuseBRDF = kD * fLambert / PI;

		// accumulate to irradiance variable
		Lo += (DiffuseBRDF + SpecBRDF) * radiance * nDotL;
	}

	vec3 ambient = vec3(0.03) * material.albedo * material.ao;
	vec3 color = ambient + Lo;

	// HDR and gamma corrections
	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0/2.2));

	FragColor = vec4(color, 1.0);
}

// uses Fresnel-Schlick approximation
vec3 Fresnel(float cosTheta, vec3 F0) {
	return F0 + (1.0 - F0) * pow(max((1.0 - cosTheta), 0.0), 5.0);
}

// uses TrowBridge-Reitz GGX
float NormalDistribution(float nDotH, float roughness) {
    float a2 = roughness * roughness;
    float denom = (nDotH * nDotH * (a2 - 1.0) + 1.0);
    return a2 / (PI * (denom * denom));
}

// uses Schlick-Beckman GGX
float GeometryEq(float dotProd, float roughness) {
	float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
	return dotProd / (dotProd * (1.0 - k) + k);
}