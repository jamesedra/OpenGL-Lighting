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

#include "../stb/stb_image.h"

constexpr int W_WIDTH = 800;
constexpr int W_HEIGHT = 600;

struct PointLightData {
	glm::vec4 positionAndConstant;
	glm::vec4 ambientAndLinear;
	glm::vec4 diffuseAndQuadratic;
	glm::vec4 specular;
};

constexpr int MAX_POINT_LIGHTS = 4;

struct PointLightsBlock {
	PointLightData pointLights[MAX_POINT_LIGHTS];
	int numPointLights = 0;
};

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
	unsigned int tex_diff = loadTexture("resources/textures/wood.png", true);
	unsigned int tex_spec = createDefaultTexture();

	Shader floorShader("shaders/base_vertex.vert", "shaders/blinn_phong.frag");

	unsigned int uboPointLights;
	glGenBuffers(1, &uboPointLights);
	glBindBuffer(GL_UNIFORM_BUFFER, uboPointLights);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(PointLightsBlock), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	unsigned int bindingPoint = 0;
	unsigned int uniformBlockIndex = glGetUniformBlockIndex(floorShader.ID, "PointLights");
	glUniformBlockBinding(floorShader.ID, uniformBlockIndex, bindingPoint);
	glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, uboPointLights);

	glm::vec3 pointLightPositions[] = {
		glm::vec3(0.0f, 0.2f, 0.0f),
		glm::vec3(5.0f, 0.2f, 5.0f),
		glm::vec3(-5.0f, 0.2f, 5.0f),
		glm::vec3(5.0f, 0.2f, -5.0f),
	};

	std::vector<PointLightData> pointLights;

	float constant = 1.0f;
	float linear = 0.22f;
	float quadratic = 0.20f;
	glm::vec3 diffuse = glm::vec3(0.5f);
	glm::vec3 specular = glm::vec3(0.5f);
	glm::vec3 ambient = glm::vec3(0.5f);

	for (int i = 0; i < sizeof(pointLightPositions) / sizeof(glm::vec3); i++) {
		pointLights.push_back({ glm::vec4(pointLightPositions[i], constant), glm::vec4(ambient, linear), glm::vec4(diffuse, quadratic), glm::vec4(specular, 1.0) });
	}

	PointLightsBlock lightsBlock;
	lightsBlock.numPointLights = static_cast<int>(pointLights.size());

	for (int i = 0; i < lightsBlock.numPointLights && i < MAX_POINT_LIGHTS; i++) {
		lightsBlock.pointLights[i].positionAndConstant = pointLights[i].positionAndConstant;
		lightsBlock.pointLights[i].diffuseAndQuadratic = pointLights[i].diffuseAndQuadratic;
		lightsBlock.pointLights[i].ambientAndLinear = pointLights[i].ambientAndLinear;
		lightsBlock.pointLights[i].specular = pointLights[i].specular;
	}

	glBindBuffer(GL_UNIFORM_BUFFER, uboPointLights);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PointLightsBlock), &lightsBlock);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// render loop
	while (!glfwWindowShouldClose(window))
	{
		// input
		processInput(window);

		// render commands
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 projection = glm::perspective(glm::radians(camera.getFOV()), (float)W_WIDTH / (float)W_HEIGHT, 0.1f, 1000.f);
		glm::mat4 view = glm::mat4(1.0f);
		glm::vec3 cameraPos = camera.getCameraPos();
		view = glm::lookAt(cameraPos, cameraPos + camera.getCameraFront(), camera.getCameraUp());

		floorShader.use();
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(10.0f, 5.0f, 10.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

		floorShader.setMat4("projection", projection);
		floorShader.setMat4("view", view);
		floorShader.setMat4("model", model);

		floorShader.setVec3("dirLight.direction", glm::vec3(0.0f, 1.0f, 0.0f));
		floorShader.setVec3("dirLight.ambient", glm::vec3(0.2f));
		floorShader.setVec3("dirLight.diffuse", glm::vec3(0.2f));
		floorShader.setVec3("dirLight.specular", glm::vec3(1.0f));

		floorShader.setInt("material.diffuse", 0);
		floorShader.setInt("material.specular", 1);
		floorShader.setFloat("material.shininess", 32.0f);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex_diff);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex_spec);
		glBindVertexArray(floorVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// checks events and swap buffers
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	glfwTerminate();

	return 0;
}

