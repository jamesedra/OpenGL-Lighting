#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[64];
uniform mat4 projection;

// based on resolution/noise size from texNoise texture
const vec2 noiseScale = vec2(1600.0/4.0, 1200.0/4.0); 
void main() {
	vec3 fragPos = texture(gPosition, TexCoords).rgb;
	vec3 normal = texture(gNormal, TexCoords).rgb;
	vec3 randomVec = texture(texNoise, TexCoords * noiseScale).rgb;

	// orthogonal basis with slight tilt from randomVec
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	int kernelSize = 64;
	float radius = 0.5;
	float occlusion = 0.0;

	for (int i = 0; i < kernelSize; ++i) {
		// transform kernel sample from tangent space to view-space
		vec3 sample = TBN * samples[i];
		
		// sample will be the offset from the current fragment position scaled from the radius
		sample = fragPos + sample * radius;

		// transform sample from view to screen space
		vec4 offset = vec4(sample, 1.0);
		offset = projection * offset;
		// perspective divide
		offset.xyz /= offset.w;
		// transform range to 0.0 - 1.0
		offset.xyz = offset.xyz * 0.5 + 0.5;

		float sampleDepth = texture(gPosition, offset.xy).z;

		float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
		float bias = 0.025;
		occlusion += (sampleDepth >= sample.z + bias ? 1.0 : 0.0) * rangeCheck;
	}

	occlusion = 1.0 - (occlusion / kernelSize); // one minus normalize based on kernel size

	FragColor = occlusion;
}