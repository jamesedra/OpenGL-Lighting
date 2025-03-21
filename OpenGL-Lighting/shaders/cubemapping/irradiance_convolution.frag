#version 330 core
out vec4 FragColor;
in vec3 localPos;

uniform samplerCube environmentMap; // from HDR

const float PI = 3.14159265359;

void main() {
	// direction = hemisphere orientation
	vec3 normal = normalize(localPos);

	vec3 irradiance = vec3(0.0);

	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = cross(up, normal);
	up = cross(normal, right);

	float sampleDelta = 0.025;
	float nrSamples = 0.0;
	
	for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)	// azimuth
	{
		for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)		// zenith
		{
			vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;

			// cos(theta) * sin(theta) are used for the weighting. 
			// weight becomes smaller when theta gets closer to 0 and 90 degrees
			irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta); 
			nrSamples++;
		}
	}

	irradiance = PI * irradiance * (1.0 / float(nrSamples));
	FragColor = vec4(irradiance, 1.0);
}