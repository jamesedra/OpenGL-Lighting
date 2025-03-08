#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoords;
	vec4 FragPosLightSpace;
	vec3 TangentLightPos;
	vec3 TangentViewPos;
	vec3 TangentFragPos;
	mat3 TBN;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 lightSpaceMatrix;

uniform vec3 viewPos;
uniform vec3 lightPos;

void main()
{
	vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
	vs_out.Normal = transpose(inverse(mat3(model))) * aNormal;
	vs_out.TexCoords = aTexCoords;
	vs_out.FragPosLightSpace = lightSpaceMatrix * model * vec4(aPos, 1.0);
	
	vec3 T = normalize(vec3(model * vec4(aTangent, 0.0)));
	vec3 N = normalize(vec3(model * vec4(aNormal, 0.0)));
	// Gram-Schmidt process - reorthogonalize
	T = normalize(T - dot(T,N) * N);
	vec3 B = cross(N, T);
	mat3 TBN = transpose(mat3(T, B, N));

	vs_out.TangentLightPos = TBN * lightPos;
	vs_out.TangentViewPos = TBN * viewPos;
	vs_out.TangentFragPos = TBN * vec3(model * vec4(aPos, 0.0));
	vs_out.TBN = TBN;

	gl_Position = projection * view * model * vec4(aPos, 1.0);
}