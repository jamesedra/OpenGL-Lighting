#version 330 core
struct Material {
	sampler2D diffuse;
	sampler2D specular;
	float shininess;
};
 
struct DirLight {
	vec3 direction;
 
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};
 
struct PointLight {
	vec4 positionAndConstant;
	vec4 ambientAndLinear;
	vec4 diffuseAndQuadratic;
	vec4 specular;
};
 
struct SpotLight {
	vec3 position;
	vec3 direction;
	float cutOff;
	float outerCutOff;
 
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};
 
 
#define NR_POINT_LIGHTS 4
 
uniform Material material;
uniform DirLight dirLight;
layout(std140) uniform PointLights{
	PointLight pointLights[NR_POINT_LIGHTS];
	int numPointLights;
};
uniform SpotLight spotLight;
uniform vec3 objectColor;
uniform vec3 viewPos;
 
out vec4 FragColor;
 
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
 
 
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
 
void main () {
	vec3 norm = normalize(Normal);
	vec3 viewDir = normalize(viewPos - FragPos);
 
	vec3 result = CalcDirLight(dirLight, norm, viewDir);
 
	for (int i = 0; i < numPointLights; i++) {
		result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
	}
	//result += CalcSpotLight(spotLight, norm, FragPos, viewDir); // not implemented yet
 
	// gamma correction
	float gamma = 2.2;
	result = pow(result, vec3(1.0/gamma));
	FragColor = vec4(result, 1.0);
}
 
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir) {
	vec3 lightDir = normalize(-light.direction);
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
 
	vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
	vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
	vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
 
	return ambient + diffuse + specular;
}
 
 
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
	vec3 lightDir = light.positionAndConstant.rgb - fragPos;
	float distance = length(lightDir);
	lightDir = normalize(lightDir);
	
	float diff = max(dot(normal, lightDir), 0.0);

	// regular phong
	// vec3 reflectDir = reflect(-lightDir, normal);
	// float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

	// blinn phong
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);

	float attenuation = 1.0 / (light.positionAndConstant.a + light.ambientAndLinear.a * distance + light.diffuseAndQuadratic.a * (distance * distance));

	vec3 ambient = light.ambientAndLinear.rgb * vec3(texture(material.diffuse, TexCoords));
	vec3 diffuse = light.diffuseAndQuadratic.rgb * diff * vec3(texture(material.diffuse, TexCoords));
	vec3 specular = light.specular.rgb * spec * vec3(texture(material.specular, TexCoords));
 
	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;
	return ambient + diffuse + specular;
}
 
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
	vec3 lightDir = normalize(light.position - fragPos);
 
	float theta = dot(lightDir, normalize(-light.direction));
	float epsilon = light.cutOff - light.outerCutOff;
	float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
 
	vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
 
	if (intensity > 0.0001)
	{
		float diff = max(dot(normal, lightDir), 0.0);
		vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
 
		vec3 viewDir = normalize(viewPos - FragPos);
		vec3 reflectDir = reflect(-lightDir, normal);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
		vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
 
		diffuse *= intensity;
        specular *= intensity;
 
		return ambient + diffuse + specular;
	}
	else return ambient;
}