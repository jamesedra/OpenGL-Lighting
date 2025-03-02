#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "modules/model.h"
#include "modules/utils.h"
#include "modules/shader.h"
#include "modules/camera.h"
#include "modules/framebuffer.h"
#include "modules/uniformbuffer.h"
#include "modules/light_types.h"
#include "modules/texture.h"

#include "../stb/stb_image.h"

constexpr int W_WIDTH = 800;
constexpr int W_HEIGHT = 600;

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

	// Shader section
	Shader shader("shaders/base_lit.vert", "shaders/blinn_phong.frag");
	Shader depthShader("shaders/simple_depth.vert", "shaders/empty.frag");
	Shader ppShader("shaders/framebuffer_quad.vert", "shaders/simple_depth.frag");

	/*
	UniformBuffer uboPointLights(sizeof(PointLightsBlock), GL_STATIC_DRAW);

	unsigned int bindingPoint = 0;
	unsigned int uniformBlockIndex = glGetUniformBlockIndex(shader.ID, "PointLights");
	glUniformBlockBinding(shader.ID, uniformBlockIndex, bindingPoint);
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

	*/

	Model object("resources/objects/backpack/backpack.obj");

	// shadow mapping
	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	Texture depthTexture(SHADOW_WIDTH, SHADOW_HEIGHT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_NEAREST, GL_REPEAT);
	Framebuffer depthFBO(SHADOW_WIDTH, SHADOW_HEIGHT, depthTexture, GL_DEPTH_ATTACHMENT);
	if (!depthFBO.isComplete()) {
		std::cout << "Frame buffer incomplete" << std::endl;
		return -1;
	}
	// no need for color buffers. set both draw and read buffer to none
	depthFBO.bind();
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	depthFBO.unbind();

	unsigned int frame = createFrameVAO();
	ppShader.use();
	ppShader.setInt("depthMap", 0);

	glm::vec3 lightPos(-2.0f, 4.0f, -1.0f);

	// prepare to bind textures
	std::vector<unsigned int> textureIDs = { tex_diff, tex_spec, depthTexture.id };

	// render loop
	while (!glfwWindowShouldClose(window))
	{
		// input
		processInput(window);

		// render commands
		
		// first pass: render to depth map
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		float near_plane = 1.0f, far_plane = 7.5f;
		glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
		glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 lightSpaceMatrix = lightProjection * lightView;

		depthShader.use();
		depthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		depthShader.setMat4("model", computeModelMatrix(glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(10.0f, 5.0f, 10.0f), 90.0f, glm::vec3(1.0f, 0.0f, 0.0f)));
		
		depthFBO.bind();
		glClear(GL_DEPTH_BUFFER_BIT);
		glBindVertexArray(floorVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		depthShader.setMat4("model", computeModelMatrix(glm::vec3(0.0f, 2.5f, 0.0f),
			glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, glm::vec3(1.0f, 0.0f, 0.0f)));
		object.Draw(depthShader);
		depthFBO.unbind();

		glViewport(0, 0, W_WIDTH, W_HEIGHT);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/* for depth debugging only 
		ppShader.use();
		glBindVertexArray(frame);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthTexture.id);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
		// */

		// second pass
		shader.use();
		shader.setMat4("projection", camera.getProjectionMatrix(W_WIDTH, W_HEIGHT, 0.1f, 1000.f));
		shader.setMat4("view", camera.getViewMatrix());
		shader.setMat4("model", computeModelMatrix(glm::vec3(0.0f, 0.0f, 0.0f), 
				glm::vec3(10.0f, 5.0f, 10.0f), -90.0f, glm::vec3(1.0f, 0.0f, 0.0f)));
		shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

		shader.setVec3("dirLight.direction", glm::normalize(-lightPos));
		shader.setVec3("dirLight.position", lightPos);
		shader.setVec3("dirLight.ambient", glm::vec3(0.05f));
		shader.setVec3("dirLight.diffuse", glm::vec3(1.0f));
		shader.setVec3("dirLight.specular", glm::vec3(1.0f));

		shader.setInt("material.diffuse", 0);
		shader.setInt("material.specular", 1);
		shader.setInt("shadowMap", 2);
		shader.setFloat("material.shininess", 64.0f);

		shader.setVec3("viewPos", camera.getCameraPos());
		bindTextures(textureIDs);

		glBindVertexArray(floorVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		shader.setMat4("model", computeModelMatrix(glm::vec3(0.0f, 2.0f, 0.0f),
			glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, glm::vec3(1.0f, 0.0f, 0.0f)));
		object.Draw(shader);

		// checks events and swap buffers
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	glfwTerminate();

	return 0;
}

