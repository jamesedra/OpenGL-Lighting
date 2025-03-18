#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../modules/model.h"
#include "../modules/utils.h"
#include "../modules/shader.h"
#include "../modules/camera.h"
#include "../modules/framebuffer.h"
#include "../modules/uniformbuffer.h"
#include "../modules/light_types.h"
#include "../modules/texture.h"

#include "../../stb/stb_image.h"
#include <random>

constexpr int W_WIDTH = 1600;
constexpr int W_HEIGHT = 1200;

int main()
{
	// initialization phase
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(W_WIDTH, W_HEIGHT, "PBR", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// Viewport setter
	glViewport(0, 0, W_WIDTH, W_HEIGHT);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glEnable(GL_DEPTH_TEST);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// Camera settings
	Camera camera(
		glm::vec3(8.0f, 8.0f, 8.0f),
		glm::vec3(-1.0f, -1.0f, -1.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		45.0f
	);
	glfwSetWindowUserPointer(window, &camera);

	// Objects
	unsigned int indicesCount;
	unsigned int sphere = createSphereVAO(indicesCount, 1.0f, 64, 64);

	// Textures
	unsigned int tex_albedo = loadTexture("resources/textures/pbr/rusted_iron/albedo.png", true, TextureColorSpace::sRGB);
	unsigned int tex_normal = loadTexture("resources/textures/pbr/rusted_iron/normal.png", true, TextureColorSpace::Linear);
	unsigned int tex_metallic = loadTexture("resources/textures/pbr/rusted_iron/metallic.png", true, TextureColorSpace::Linear);
	unsigned int tex_roughness = loadTexture("resources/textures/pbr/rusted_iron/roughness.png", true, TextureColorSpace::Linear);
	unsigned int tex_ao = loadTexture("resources/textures/pbr/rusted_iron/ao.png", true, TextureColorSpace::Linear);

	std::vector<unsigned int> sphereTex = { tex_albedo, tex_normal, tex_metallic, tex_roughness, tex_ao };

	Shader PBRShader("shaders/base_lit.vert", "shaders/pbr/pbr_textures_wnormals.frag");

	glm::vec3 lightPositions[4] = {
		glm::vec3(1.5f, 0.0f, 1.5f),
		glm::vec3(-1.5f, 0.0f, 1.5f),
		glm::vec3(1.5f, 0.0f, -1.5f),
		glm::vec3(-1.5f, 0.0f, -1.5f)
	};

	glm::vec3 lightColors[4] = {
		glm::vec3(10.0f, 10.0f, 10.0f),
		glm::vec3(10.0f, 5.0f, 0.0f),
		glm::vec3(5.0f, 5.0f, 10.0f),
		glm::vec3(5.0f, 10.0f, 5.0f)
	};

	// render loop
	while (!glfwWindowShouldClose(window))
	{
		// input
		processInput(window);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		PBRShader.use();
		PBRShader.setMat4("projection", camera.getProjectionMatrix(W_WIDTH, W_HEIGHT, 0.1f, 1000.0f));
		PBRShader.setMat4("view", camera.getViewMatrix());

		float time = glfwGetTime();
		PBRShader.setMat4("model", computeModelMatrix(glm::vec3(0.0f), glm::vec3(1.0f, 1.0f, 1.0f), fmod(time * 30.0f, 360.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
		PBRShader.setVec3("viewPos", camera.getCameraPos());

		// material uniforms
		// PBRShader.setVec3("material.albedo", glm::vec3(1.0f, 0.0f, 0.0f));
		// PBRShader.setFloat("material.metallic", 1.0f);
		// PBRShader.setFloat("material.roughness", 0.2f);
		// PBRShader.setFloat("material.ao", 0.0f);
		PBRShader.setInt("material.albedoMap", 0);
		PBRShader.setInt("material.normalMap", 1);
		PBRShader.setInt("material.metallicMap", 2);
		PBRShader.setInt("material.roughnessMap", 3);
		PBRShader.setInt("material.aoMap", 4);
		bindTextures(sphereTex);

		// light uniforms
		for (int i = 0; i < 4; ++i) {
			PBRShader.setVec3("lights[" + std::to_string(i) + "].position", lightPositions[i]);
			PBRShader.setVec3("lights[" + std::to_string(i) + "].color", lightColors[i]);
		}
		glBindVertexArray(sphere);
		glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, 0);

		// checks events and swap buffers
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	glfwTerminate();

	return 0;
}