#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

struct Light {
	vec3 position; // stores direction if it's a directional Light
	vec3 color;
	float intensity;
};
uniform Light dirLight;

struct Material {
	vec3 baseColor;
	float roughness;
};
uniform Material material;
uniform vec3 viewPos;

float Fresnel(float vDotH);
float NormalDistribution(float nDotH);
float GeometryEq(float dotProd);

const float PI = 3.14159;

void main () {
	// reflectance equation (simplified with only one directional light source [summation needed if multiple])
	// FragColor = (BRDF * Light Intensity * dot(normal, light direction)
	// where BRDF is DiffuseBRDF + SpecularBRDF
	// where DiffuseBRDF is kD * lambertianDiffuse
	// and SpecularBRDF is ks * cookTorrance (or some variation)

	vec3 l = normalize(dirLight.position);
	vec3 n = Normal;
	vec3 v = normalize(viewPos - FragPos);		// view direction
	vec3 h = normalize(v + l);  // halfway vector

	float nDotL = max(dot(n, l), 0.0);
	float vDotH = max(dot(v, h), 0.0);
	float nDotH = max(dot(n, h), 0.0);
	float nDotV = max(dot(n, v), 0.0);

	vec3 lightIntensity = dirLight.color * dirLight.intensity;

	// BRDF start
	float D = NormalDistribution(nDotH);
	float G = GeometryEq(nDotL) * GeometryEq(nDotV);
	float F = Fresnel(vDotH);

	float kS = F;
	float kD = 1.0 - kS;

	float SpecBRDF_nom = D * G * F;
	float SpecBRDF_denom = 4.0 * nDotV * nDotL + 0.0001; // avoid divide by 0
	float SpecBRDF = SpecBRDF_nom / SpecBRDF_denom;

	vec3 fLambertian = material.baseColor;
	vec3 DiffuseBRDF = kD * fLambertian / PI;

	// Rendering Equation
	vec3 result = (DiffuseBRDF + SpecBRDF) * lightIntensity * nDotL;

	FragColor = vec4(result, 1.0);
}

float Fresnel(float vDotH) {
	// uses Fresnel-Schlick approximation, not for metals
	float F0 = 0.04;
	return F0 + (1.0 - F0) * pow(max((1.0 - vDotH), 0.0), 5.0);
}

float NormalDistribution(float nDotH) {
	// uses TrowBridge-Reitz GGX
    float a2 = material.roughness * material.roughness;
    float denom = (nDotH * nDotH * (a2 - 1.0) + 1.0);
    return a2 / (PI * (denom * denom));
	
}
float GeometryEq(float dotProd) {
	// uses Schlick-Beckman GGX
	float k = (material.roughness + 1.0) * (material.roughness + 1.0) / 8.0;
	return dotProd / (dotProd * (1.0 - k) + k);
}
