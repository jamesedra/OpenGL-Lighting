#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "utils.h"
#include "camera.h"

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// mouse stuff
bool firstMouse = false;
float lastX = 400;
float lastY = 300;
float pitch = 0.0f;
float yaw = -90.0f;
float zoom = 45.0f;


void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
	if (!camera) return;

	float currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;


	const float cameraSpeed = 12.5f * deltaTime; // Adjust as needed.

	// Update camera position based on key input:
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		camera->setCameraPos(camera->getCameraPos() + cameraSpeed * camera->getCameraFront());
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		camera->setCameraPos(camera->getCameraPos() - cameraSpeed * camera->getCameraFront());
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		// Calculate the right vector: normalized cross product of front and up.
		glm::vec3 right = glm::normalize(glm::cross(camera->getCameraFront(), camera->getCameraUp()));
		camera->setCameraPos(camera->getCameraPos() - right * cameraSpeed);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		glm::vec3 right = glm::normalize(glm::cross(camera->getCameraFront(), camera->getCameraUp()));
		camera->setCameraPos(camera->getCameraPos() + right * cameraSpeed);
	}
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
	if (!camera) return;

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	const float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f) pitch = 89.0f;
	if (pitch < -89.0f) pitch = -89.0f;

	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	camera->setCameraFront(glm::normalize(direction));
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
	if (!camera) return;

	zoom -= (float)yoffset;
	if (zoom < 1.0f) zoom = 1.0f;
	if (zoom > 45.0f) zoom = 45.0f;
	
	camera->setFOV(zoom);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

unsigned int loadCubemap(std::vector<std::string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

unsigned int createDefaultTexture()
{
	unsigned char whitePixel[4] = { 255, 255, 255, 255 };
	Texture whiteTex(1, 1, GL_RGBA, GL_RGBA, whitePixel);
	whiteTex.setTexFilter(GL_LINEAR);

	return whiteTex.id;
}

unsigned int loadTexture(const char* path, bool flipVertically, TextureColorSpace space)
{
	stbi_set_flip_vertically_on_load(true);

	int width, height, nrChannels;
	unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
	if (!data)
	{
		std::cout << "Failed to load texture" << std::endl;
		stbi_image_free(data);
		return 0;
	}
	
	GLenum baseFormat = GL_RGB;
	if (nrChannels == 1)
		baseFormat = GL_RED;
	else if (nrChannels == 3)
		baseFormat = GL_RGB;
	else if (nrChannels == 4)
		baseFormat = GL_RGBA;

	GLenum internalFormat = baseFormat;

	if (space == TextureColorSpace::sRGB) {
		if (baseFormat == GL_RGB)
			internalFormat = GL_SRGB;
		else if (baseFormat == GL_RGBA)
			internalFormat = GL_SRGB_ALPHA;
	}

	Texture tex(width, height, internalFormat, baseFormat, GL_LINEAR, GL_REPEAT, data);
	tex.genMipMap();

	stbi_image_free(data);

	return tex.id;
}

unsigned int createCubeVAO()
{
	float vertices[] = {
		// positions          // normals           // texture coords
		-0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   1.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   1.0f,  1.0f,
		 0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   1.0f,  1.0f,
		-0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   0.0f,  1.0f,
		-0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   0.0f,  0.0f,

		-0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   0.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   1.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   1.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   0.0f,  0.0f,

		-0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,   1.0f,  1.0f,
		-0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,   0.0f,  1.0f,
		-0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,   0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   1.0f,  0.0f,

		 0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,   1.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   1.0f,  1.0f,
		 0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   0.0f,  1.0f,
		 0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   0.0f,  1.0f,
		 0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f,   0.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,   1.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   0.0f,  1.0f,
		 0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   1.0f,  1.0f,
		 0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   0.0f,  1.0f,

		-0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,   0.0f,  1.0f,
		 0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,   1.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,   0.0f,  1.0f
	};

	// cube object
	unsigned int cubeVAO;
	glGenVertexArrays(1, &cubeVAO);

	unsigned int VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindVertexArray(cubeVAO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return cubeVAO;
}

unsigned int createQuadVAO()
{
	float vertices[] = {
		// positions          // normals          // texture coords
		-1.0f,  1.0f, 0.0f,    0.0f, 0.0f, 1.0f,   0.0f, 1.0f,  // top left
		-1.0f, -1.0f, 0.0f,    0.0f, 0.0f, 1.0f,   0.0f, 0.0f,  // bottom left
		 1.0f, -1.0f, 0.0f,    0.0f, 0.0f, 1.0f,   1.0f, 0.0f,  // bottom right

		-1.0f,  1.0f, 0.0f,    0.0f, 0.0f, 1.0f,   0.0f, 1.0f,  // top left
		 1.0f, -1.0f, 0.0f,    0.0f, 0.0f, 1.0f,   1.0f, 0.0f,  // bottom right
		 1.0f,  1.0f, 0.0f,    0.0f, 0.0f, 1.0f,   1.0f, 1.0f   // top right
	};

	glm::vec3 positions1[3] = { getVertexPosition(vertices, 0), getVertexPosition(vertices, 8), getVertexPosition(vertices, 16) };
	glm::vec3 positions2[3] = { getVertexPosition(vertices, 24), getVertexPosition(vertices, 32), getVertexPosition(vertices, 40) };
	glm::vec2 uv1[3] = { getUVPosition(vertices, 6), getUVPosition(vertices, 14), getUVPosition(vertices, 22) };
	glm::vec2 uv2[3] = { getUVPosition(vertices, 30), getUVPosition(vertices, 38), getUVPosition(vertices, 46) };

	glm::mat2x3 TBMatrix1 = getTangentBitangentMatrix(positions1, uv1);
	glm::mat2x3 TBMatrix2 = getTangentBitangentMatrix(positions2, uv2);

	glm::vec3 tangent1 = TBMatrix1[0];
	glm::vec3 bitangent1 = TBMatrix1[1];
	glm::vec3 tangent2 = TBMatrix2[0];
	glm::vec3 bitangent2 = TBMatrix2[1];

	glm::vec3 tanBitan[12] = {
		tangent1, bitangent1,
		tangent1, bitangent1,
		tangent1, bitangent1,
		tangent2, bitangent2,
		tangent2, bitangent2,
		tangent2, bitangent2
	};

	unsigned int quadVAO;
	glGenVertexArrays(1, &quadVAO);
	glBindVertexArray(quadVAO);

	unsigned int VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	unsigned int tanBitanVBO;
	glGenBuffers(1, &tanBitanVBO);
	glBindBuffer(GL_ARRAY_BUFFER, tanBitanVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tanBitan), tanBitan, GL_STATIC_DRAW);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec3), (void*)(0 * sizeof(glm::vec3)));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec3), (void*)(sizeof(glm::vec3)));
	glEnableVertexAttribArray(4);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return quadVAO;
}

glm::mat2x3 getTangentBitangentMatrix(glm::vec3 positions[3], glm::vec2 texCoords[3]) {
	glm::vec3 edge1 = positions[1] - positions[0];
	glm::vec3 edge2 = positions[2] - positions[0];
	glm::vec2 deltaUV1 = texCoords[1] - texCoords[0];
	glm::vec2 deltaUV2 = texCoords[2] - texCoords[0];

	glm::vec3 tangent, bitangent;

	float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
	tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
	bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

	glm::mat2x3 tangentBitangent;
	tangentBitangent[0] = tangent;  
	tangentBitangent[1] = bitangent;

	return tangentBitangent;
}

unsigned int createFrameVAO()
{
	float quadVertices[] = {
		// positions(2) : uvs(2)
		-1.0f, 1.0f, 0.0f, 1.0f,
		-1.0, -1.0f, 0.0f, 0.0f,
		 1.0f,-1.0f, 1.0f, 0.0f,

		-1.0f, 1.0f, 0.0f, 1.0f,
		 1.0f,-1.0f, 1.0f, 0.0f,
		 1.0f, 1.0f, 1.0f, 1.0f
	};

	unsigned int quadVAO;
	glGenVertexArrays(1, &quadVAO);

	unsigned int quadVBO;
	glGenBuffers(1, &quadVBO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
	glBindVertexArray(quadVAO);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return quadVAO;
}

glm::mat4 computeModelMatrix(const glm::vec3& position, const glm::vec3& scale, float angleDegrees, const glm::vec3& rotationAxis)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, position);
	model = glm::scale(model, scale);
	model = glm::rotate(model, glm::radians(angleDegrees), rotationAxis);
	return model;
}

void bindTextures(const std::vector<unsigned int>& textures, GLenum textureTarget, unsigned int startUnit)
{
	for (size_t i = 0; i < textures.size(); ++i)
	{
		glActiveTexture(startUnit + i);
		glBindTexture(textureTarget, textures[i]);
	}
}

glm::vec3 getVertexPosition(const float* vertices, int index) {
	return glm::vec3(vertices[index], vertices[index + 1], vertices[index + 2]);
}

glm::vec2 getUVPosition(const float* vertices, int index) {
	return glm::vec2(vertices[index], vertices[index + 1]);
}