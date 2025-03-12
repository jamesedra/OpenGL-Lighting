#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform vec3 Color;

void main()
{
    float dist = distance(TexCoords, vec2(0.5, 0.5));

    float alpha = 1.0 - smoothstep(0.3, 0.5, dist);
    FragColor = vec4(Color, alpha);
}