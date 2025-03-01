#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "modules/Model.h"
#include "modules/utils.h"
#include "modules/shader.h"
#include "modules/camera.h"
#include "modules/framebuffer.h"
#include "modules/uniformbuffer.h"
#include "modules/light_types.h"

#include "../stb/stb_image.h"

constexpr int W_WIDTH = 1200;
constexpr int W_HEIGHT = 800;

int main()
{
	// initialization phase
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// create a window object
	GLFWwindow* window = glfwCreateWindow(W_WIDTH, W_HEIGHT, "Test Window", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	// tell GLFW to make the window the context of the current thread
	glfwMakeContextCurrent(window);

	// checks if GLAD loader is validated
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// viewport setter
	glViewport(0, 0, W_WIDTH, W_HEIGHT);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glEnable(GL_DEPTH_TEST);
	// glEnable(GL_FRAMEBUFFER_SRGB);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// camera settings
	Camera camera(
		glm::vec3(0.0f, 1.0f, 3.0f),
		glm::vec3(0.0f, 0.0f, -1.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		45.0f
	);
	glfwSetWindowUserPointer(window, &camera);

	unsigned int floorVAO = createQuadVAO();
	unsigned int tex_diff = loadTexture("resources/textures/wood.png", true, TextureColorSpace::sRGB);
	unsigned int tex_spec = createDefaultTexture();

	Shader floorShader("shaders/base_vertex.vert", "shaders/blinn_phong.frag");

	UniformBuffer uboPointLights(sizeof(PointLightsBlock), GL_STATIC_DRAW);

	unsigned int bindingPoint = 0;
	unsigned int uniformBlockIndex = glGetUniformBlockIndex(floorShader.ID, "PointLights");
	glUniformBlockBinding(floorShader.ID, uniformBlockIndex, bindingPoint);
	uboPointLights.bindBufferBase(bindingPoint);

	glm::vec3 pointLightPositions[] = {
		glm::vec3(0.0f, 1.5f, 0.25f),
		glm::vec3(5.0f, 0.1f, 5.0f),
		glm::vec3(-5.0f, 0.1f, 5.0f),
		glm::vec3(5.0f, 0.1f, -5.0f),
	};

	std::vector<PointLightData> pointLights;
	float constant = 1.0f;
	float linear = 0.7f;
	float quadratic = 1.8f;
	glm::vec3 diffuse = glm::vec3(0.7f);
	glm::vec3 specular = glm::vec3(1.0f);
	glm::vec3 ambient = glm::vec3(0.7f);

	for (int i = 0; i < sizeof(pointLightPositions) / sizeof(glm::vec3); i++) {
		pointLights.push_back({ glm::vec4(pointLightPositions[i], constant), glm::vec4(ambient, linear), glm::vec4(diffuse, quadratic), glm::vec4(specular, 1.0) });
	}

	PointLightsBlock lightsBlock(pointLights);
	uboPointLights.setData(&lightsBlock, sizeof(PointLightsBlock));

	// prepare to bind textures
	std::vector<unsigned int> textureIDs = { tex_diff, tex_spec };

	Model object("resources/objects/backpack/backpack.obj");

	// render loop
	while (!glfwWindowShouldClose(window))
	{
		// input
		processInput(window);

		// render commands
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	

		floorShader.use();
		floorShader.setMat4("projection", camera.getProjectionMatrix(W_WIDTH, W_HEIGHT, 0.1f, 1000.f));
		floorShader.setMat4("view", camera.getViewMatrix());
		floorShader.setMat4("model", computeModelMatrix(glm::vec3(0.0f, 0.0f, 0.0f), 
				glm::vec3(10.0f, 5.0f, 10.0f), 90.0f, glm::vec3(1.0f, 0.0f, 0.0f)));

		floorShader.setVec3("dirLight.direction", glm::vec3(0.0f, 1.0f, 0.0f));
		floorShader.setVec3("dirLight.ambient", glm::vec3(0.1f));
		floorShader.setVec3("dirLight.diffuse", glm::vec3(0.1f));
		floorShader.setVec3("dirLight.specular", glm::vec3(1.0f));

		floorShader.setInt("material.diffuse", 0);
		floorShader.setInt("material.specular", 1);
		floorShader.setFloat("material.shininess", 32.0f);

		bindTextures(textureIDs);

		glBindVertexArray(floorVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		floorShader.setMat4("model", computeModelMatrix(glm::vec3(0.0f, 2.5f, 0.0f),
			glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, glm::vec3(1.0f, 0.0f, 0.0f)));
		object.Draw(floorShader);

		// checks events and swap buffers
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	glfwTerminate();

	return 0;
}

