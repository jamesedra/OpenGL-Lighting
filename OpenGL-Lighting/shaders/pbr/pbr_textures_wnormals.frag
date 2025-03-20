#version 330 core

out vec4 FragColor;

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

uniform vec3 viewPos;

struct Material {
	sampler2D albedoMap;
	sampler2D normalMap;
	sampler2D metallicMap;
	sampler2D roughnessMap;
	sampler2D aoMap;

	// vec3 albedo;
	// float metallic;
	// float roughness;
	// float ao;
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

	vec3 albedo = pow(texture(material.albedoMap, fs_in.TexCoords).rgb, vec3(2.2));
	float metallic = texture(material.metallicMap,  fs_in.TexCoords).r;
	float roughness = texture(material.roughnessMap,  fs_in.TexCoords).r;
	float ao = texture(material.aoMap,  fs_in.TexCoords).r;

	vec3 n = texture(material.normalMap, fs_in.TexCoords).rgb;
	n = normalize(n * 2.0 - 1.0);
	n = normalize(fs_in.nonTransTBN * n);

	vec3 v = normalize(viewPos -  fs_in.FragPos);	// view dir

	roughness = max(roughness, 0.0001);

	vec3 Lo = vec3(0.0);	// Irradiance

	for (int i = 0; i < 4; i++) {
		vec3 l = normalize(lights[i].position -  fs_in.FragPos);	// light dir
		vec3 h = normalize(v + l);	// halfway vector

		float distance = length(lights[i].position - fs_in.FragPos);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = lights[i].color * attenuation;

		// Dot product setup
		float nDotL = max(dot(n, l), 0.0);
		float vDotH = max(dot(v, h), 0.0);
		float nDotH = max(dot(n, h), 0.0);
		float nDotV = max(dot(n, v), 0.0);

		// Specular BRDF
		vec3 F0 = vec3(0.04);
		F0 = mix(F0, albedo, metallic);
		
		vec3 F = Fresnel(vDotH, F0);
		float D = NormalDistribution(nDotH, roughness);
		float G = GeometryEq(nDotL, roughness) * GeometryEq(nDotV, roughness);

		vec3 SpecBRDF_nom = D * G * F;
		float SpecBRDF_denom = 4.0 * nDotV * nDotL;
		vec3 SpecBRDF = SpecBRDF_nom / max(SpecBRDF_denom, 0.001);

		// Diffuse BRDF
		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - metallic;		// decreases diffuse reflections as metallic value increases.
		
		vec3 fLambert = albedo;
		vec3 DiffuseBRDF = kD * fLambert / PI;

		// accumulate to irradiance variable
		Lo += (DiffuseBRDF + SpecBRDF) * radiance * nDotL;
	}

	vec3 ambient = vec3(0.03) * albedo * ao;
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