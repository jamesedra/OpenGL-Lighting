#version 330 core

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;


void main()
{
	FragColor = vec4(15.0, 15.0, 15.0, 15.0);
	BrightColor = vec4(FragColor.rgb, 1.0);
}