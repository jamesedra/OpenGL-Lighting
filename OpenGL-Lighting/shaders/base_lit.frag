#version 330 core

in VS_OUT {
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoords;
	vec4 FragPosLightSpace;
	vec3 TangentLightPos;
	vec3 TangentViewPos;
	vec3 TangentFragPos;
} fs_in;

out vec4 FragColor;

struct Material {
	sampler2D diffuse;
	sampler2D specular;
	sampler2D normal;
	sampler2D depth;
	float shininess;
};

struct DirLight {
	vec3 direction;
	vec3 position;
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

uniform Material material;
uniform DirLight dirLight;
uniform PointLight pointLight;

uniform sampler2D dirShadowMap;
uniform samplerCube pointShadowMap;
uniform vec3 viewPos;

uniform float far_plane;
uniform float height_scale;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec2 texCoords);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
float ShadowDirCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir);
float ShadowPointCalculation(PointLight light, vec3 fragPos);
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir);
vec2 SteepParallaxMapping(vec2 texCoords, vec3 viewDir);
vec2 ParallaxOcclusionMapping(vec2 texCoords, vec3 viewDir);

void main () {
	vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);

	// vec3 norm = normalize(fs_in.Normal);
	vec2 texCoords = ParallaxOcclusionMapping(fs_in.TexCoords, viewDir);

	if (texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0) discard;

	vec3 norm = texture(material.normal, texCoords).rgb;
	norm = normalize(norm * 2.0 - 1.0);
	
	vec3 result = CalcDirLight(dirLight, norm, viewDir, texCoords); 

	float gamma = 2.2;
	result = pow(result, vec3(1.0/gamma));
	FragColor = vec4(result, 1.0);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec2 texCoords) {

	vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
	float diff = max(dot(normal, lightDir), 0.0);

	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
 
	float shadow = ShadowDirCalculation(fs_in.FragPosLightSpace, normal, lightDir);
	
	vec3 ambient = light.ambient * vec3(texture(material.diffuse, texCoords));
	vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, texCoords));
	vec3 specular = light.specular * spec * vec3(texture(material.specular, texCoords));

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

	float closestDepth = texture(dirShadowMap, projCoords.xy).r;
	float currentDepth = projCoords.z;

	float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

	if (projCoords.z > 1.0) return 0.0; // for coordinates farther than the light's far plane

	// percentage-closer filtering
	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(dirShadowMap, 0);
	for (int x =-1; x <= 1; ++x) {
		for (int y =-1; y <= 1; ++y) {
			float pcfDepth = texture(dirShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
			shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
		}
	}

	shadow /= 9.0;

	return shadow;
}

float ShadowPointCalculation(PointLight light, vec3 fragPos)
{
	vec3 fragToLight = fragPos - light.positionAndConstant.rgb;
	float closestDepth = texture(pointShadowMap, fragToLight).r;

	closestDepth *= far_plane; // transform range [0, 1] to [0, far_plane]

	float currentDepth = length(fragToLight);

	float bias = 0.05;
	float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

	return shadow;
}

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir) {
	float height = texture(material.depth, texCoords).r;
	vec2 p = viewDir.xy / viewDir.z * (height * height_scale);
	return texCoords - p;
}

vec2 SteepParallaxMapping(vec2 texCoords, vec3 viewDir) {
	const float minLayers = 8.0;
	const float maxLayers = 32.0;

	float numLayers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 0.1), viewDir), 0.0));
	float layerDepth = 1.0 / numLayers;
	float currentLayerDepth = 0.0;
	vec2 P = viewDir.xy * height_scale;
	vec2 deltaTexCoords = P / numLayers;

	vec2 currTexCoords = texCoords;
	float currDepthMapValue = texture(material.depth, currTexCoords).r;

	while (currentLayerDepth < currDepthMapValue) {
		// shift uv
		currTexCoords -= deltaTexCoords;
		// get the shifted uv depth value
		currDepthMapValue = texture(material.depth, currTexCoords).r;
		// iterate to the next depth layer
		currentLayerDepth += layerDepth;
	}

	return currTexCoords;
}

vec2 ParallaxOcclusionMapping(vec2 texCoords, vec3 viewDir) {
	const float minLayers = 8.0;
	const float maxLayers = 32.0;

	float numLayers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 0.1), viewDir), 0.0));
	float layerDepth = 1.0 / numLayers;
	float currentLayerDepth = 0.0;
	vec2 P = viewDir.xy * height_scale;
	vec2 deltaTexCoords = P / numLayers;

	vec2 currTexCoords = texCoords;
	float currDepthMapValue = texture(material.depth, currTexCoords).r;

	while (currentLayerDepth < currDepthMapValue) {
		// shift uv
		currTexCoords -= deltaTexCoords;
		// get the shifted uv depth value
		currDepthMapValue = texture(material.depth, currTexCoords).r;
		// iterate to the next depth layer
		currentLayerDepth += layerDepth;
	}

	vec2 prevTexCoords = currTexCoords + deltaTexCoords;

	float afterDepth = currDepthMapValue - currentLayerDepth;
	float beforeDepth = texture(material.depth, prevTexCoords).r - currentLayerDepth + layerDepth;

	float weight = afterDepth / (afterDepth - beforeDepth);
	vec2 finalTexCoords = prevTexCoords * weight + currTexCoords * (1.0 - weight);

	return finalTexCoords;
}