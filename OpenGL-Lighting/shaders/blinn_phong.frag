#version 330 core
struct Material {
	sampler2D diffuse;
	sampler2D specular;
	float shininess;
};
 
struct DirLight {
	vec3 direction;
	vec3 position; // for testing only
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
 
out vec4 FragColor;
 
in VS_OUT {
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoords;
	vec4 FragPosLightSpace;
} fs_in;
 
 #define NR_POINT_LIGHTS 4
 
uniform Material material;
uniform DirLight dirLight;
layout(std140) uniform PointLights{
	PointLight pointLights[NR_POINT_LIGHTS];
	int numPointLights;
};

uniform sampler2D shadowMap;
uniform samplerCube shadowCubemap;
uniform vec3 objectColor;
uniform vec3 viewPos;

uniform float far_plane;
 
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
float ShadowDirCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir);
float ShadowPointCalculation(PointLight light, vec3 fragPos);
 
void main () {
	vec3 norm = normalize(fs_in.Normal);
	vec3 viewDir = normalize(viewPos - fs_in.FragPos);
 
	// vec3 result = vec3(0.0); //CalcDirLight(dirLight, norm, viewDir);
 
	// for (int i = 0; i < numPointLights; i++) {
		vec3 result = CalcPointLight(pointLights[0], norm, fs_in.FragPos, viewDir);
	// }
	// gamma correction
	// float gamma = 2.2;
    // result = pow(result, vec3(1.0/gamma));

	FragColor = vec4(result, 1.0);
}
 
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir) {
	vec3 lightDir = normalize(light.position - fs_in.FragPos);
	float diff = max(dot(normal, lightDir), 0.0);

	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
 
	float shadow = ShadowDirCalculation(fs_in.FragPosLightSpace, normal, lightDir);
	
	vec3 ambient = light.ambient * vec3(texture(material.diffuse, fs_in.TexCoords));
	vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, fs_in.TexCoords));
	vec3 specular = light.specular * spec * vec3(texture(material.specular, fs_in.TexCoords));
	
	return (ambient + (1.0 - shadow) * (diffuse + specular));
}
 
 
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
	vec3 lightDir = light.positionAndConstant.rgb - fragPos;
	float distance = length(lightDir);
	lightDir = normalize(lightDir);
	
	float diff = max(dot(normal, lightDir), 0.0);

	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);

	float shadow = ShadowPointCalculation(light, fragPos);

	float attenuation = 1.0 / (light.positionAndConstant.a + light.ambientAndLinear.a * distance + light.diffuseAndQuadratic.a * (distance * distance));

	vec3 ambient = light.ambientAndLinear.rgb * vec3(texture(material.diffuse, fs_in.TexCoords));
	vec3 diffuse = light.diffuseAndQuadratic.rgb * diff * vec3(texture(material.diffuse, fs_in.TexCoords));
	vec3 specular = light.specular.rgb * spec * vec3(texture(material.specular, fs_in.TexCoords));
 
	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;

	return ambient + (1.0 - shadow) * (diffuse + specular);
}

float ShadowDirCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords = projCoords * 0.5 + 0.5;

	float closestDepth = texture(shadowMap, projCoords.xy).r;
	float currentDepth = projCoords.z;

	float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

	if (projCoords.z > 1.0) return 0.0; // for coordinates farther than the light's far plane

	// percentage-closer filtering
	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	for (int x =-1; x <= 1; ++x) {
		for (int y =-1; y <= 1; ++y) {
			float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
			shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
		}
	}

	shadow /= 9.0;

	return shadow;
}

float ShadowPointCalculation(PointLight light, vec3 fragPos)
{
	vec3 fragToLight = fragPos - light.positionAndConstant.rgb;
	float closestDepth = texture(shadowCubemap, fragToLight).r;

	closestDepth *= far_plane; // transform range [0, 1] to [0, far_plane]

	float currentDepth = length(fragToLight);

	float bias = 0.05;
	float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

	return shadow;
}