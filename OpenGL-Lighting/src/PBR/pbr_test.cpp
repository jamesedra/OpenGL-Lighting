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

	GLFWwindow* window = glfwCreateWindow(W_WIDTH, W_HEIGHT, "SSAO", NULL, NULL);
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

	Shader PBRShader("shaders/base_vertex.vert", "shaders/pbr/pbr_test.frag");

	// render loop
	while (!glfwWindowShouldClose(window))
	{
		// input
		processInput(window);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		PBRShader.use();
		PBRShader.setMat4("projection", camera.getProjectionMatrix(W_WIDTH, W_HEIGHT, 0.1f, 1000.0f));
		PBRShader.setMat4("view", camera.getViewMatrix());
		PBRShader.setMat4("model", computeModelMatrix(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f)));
		PBRShader.setVec3("viewPos", camera.getCameraPos());

		float time = glfwGetTime();
		glm::vec3 lightPos = glm::vec3(0.5f);
		// light uniforms
		PBRShader.setVec3("dirLight.position", glm::vec3(sin(time + lightPos.x), lightPos.y, cos(time + lightPos.z)));

		PBRShader.setVec3("dirLight.color", glm::vec3(1.0f));
		PBRShader.setFloat("dirLight.intensity", 2.0f);

		// material uniforms
		PBRShader.setVec3("material.baseColor", glm::vec3(1.0f, 0.0f, 1.0f));
		PBRShader.setFloat("material.roughness", 0.1f);

		glBindVertexArray(sphere);
		glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, 0);
		
		// checks events and swap buffers
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	glfwTerminate();

	return 0;
}