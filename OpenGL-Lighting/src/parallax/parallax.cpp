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

constexpr int W_WIDTH = 1600;
constexpr int W_HEIGHT = 1200;

int parallax_main() {
	// initialization phase
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(W_WIDTH, W_HEIGHT, "Parallax Mapping", NULL, NULL);
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

	// viewport setter
	glViewport(0, 0, W_WIDTH, W_HEIGHT);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// camera settings
	Camera camera(
		glm::vec3(8.0f, 8.0f, 8.0f),
		glm::vec3(-1.0f, -1.0f, -1.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		45.0f
	);
	glfwSetWindowUserPointer(window, &camera);

	Shader floorShader("shaders/base_lit.vert", "shaders/base_lit.frag");
	Shader depthDirShader("shaders/simple_depth.vert", "shaders/empty.frag");
	Shader cyborgShader("shaders/base_lit.vert", "shaders/material_lit.frag");

	// directional shadow mapping
	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	Texture depthTexture(SHADOW_WIDTH, SHADOW_HEIGHT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT);
	depthTexture.setTexFilter(GL_NEAREST);
	depthTexture.setTexWrap(GL_CLAMP_TO_BORDER);
	depthTexture.bind();
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	depthTexture.unbind();

	Framebuffer depthFBO(SHADOW_WIDTH, SHADOW_HEIGHT, depthTexture, GL_DEPTH_ATTACHMENT);
	if (!depthFBO.isComplete())
	{
		std::cout << "Frame buffer incomplete" << std::endl;
		return -1;
	}
	// no need for color buffers. set both draw and read buffer to none
	depthFBO.bind();
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	depthFBO.unbind();

	unsigned int floorVAO = createQuadVAO();
	unsigned int tex_diff = loadTexture("resources/textures/bricks2.jpg", true, TextureColorSpace::sRGB);
	unsigned int tex_norm = loadTexture("resources/textures/bricks2_normal.jpg", true, TextureColorSpace::Linear);
	unsigned int tex_spec = createDefaultTexture();
	unsigned int tex_disp = loadTexture("resources/textures/bricks2_disp.jpg", true, TextureColorSpace::Linear);

	std::vector<unsigned int> textureIDs = { tex_diff, tex_spec, tex_norm, tex_disp, depthTexture.id };
	glm::vec3 dirLightPos(5.0f, 4.0f, 5.0f);
	float near_plane = 1.0f, far_plane = 15.0f;

	Model cyborg("resources/objects/cyborg/cyborg.obj");

	// render loop
	while (!glfwWindowShouldClose(window))
	{
		// input
		processInput(window);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// light movement test
		float time = glfwGetTime();
		dirLightPos.x = 5.0f * sin(time);
		dirLightPos.z = 5.0f * cos(time);

		// render commands
		glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
		glm::mat4 lightView = glm::lookAt(dirLightPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 lightSpaceMatrix = lightProjection * lightView;

		glCullFace(GL_FRONT);
		depthDirShader.use();
		depthDirShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		depthDirShader.setMat4("model", computeModelMatrix(glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(10.0f, 5.0f, 10.0f), -90.0f, glm::vec3(1.0f, 0.0f, 0.0f)));

		depthFBO.bind();
		glClear(GL_DEPTH_BUFFER_BIT);
		glBindVertexArray(floorVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		depthDirShader.setMat4("model", computeModelMatrix(glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, glm::vec3(1.0f, 0.0f, 0.0f)));
		cyborg.Draw(depthDirShader);
		depthFBO.unbind();
		glCullFace(GL_BACK);


		glViewport(0, 0, W_WIDTH, W_HEIGHT);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		floorShader.use();
		floorShader.setMat4("projection", camera.getProjectionMatrix(W_WIDTH, W_HEIGHT, 0.1f, 1000.f));
		floorShader.setMat4("view", camera.getViewMatrix());
		floorShader.setMat4("model", computeModelMatrix(glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(10.0f, 10.0f, 10.0f), -90.0f, glm::vec3(1.0f, 0.0f, 0.0f)));
		floorShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		floorShader.setVec3("lightPos", dirLightPos);
		floorShader.setVec3("dirLight.position", dirLightPos);
		floorShader.setVec3("dirLight.ambient", glm::vec3(0.05f));
		floorShader.setVec3("dirLight.diffuse", glm::vec3(0.5f));
		floorShader.setVec3("dirLight.specular", glm::vec3(0.3f));

		floorShader.setInt("material.diffuse", 0);
		floorShader.setInt("material.specular", 1);
		floorShader.setInt("material.normal", 2);
		floorShader.setInt("material.depth", 3);
		floorShader.setInt("dirShadowMap", 4);

		floorShader.setFloat("material.shininess", 64.0f);
		floorShader.setVec3("viewPos", camera.getCameraPos());
		floorShader.setFloat("height_scale", 0.05f);

		bindTextures(textureIDs);
		glBindVertexArray(floorVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		cyborgShader.use();
		cyborgShader.setMat4("projection", camera.getProjectionMatrix(W_WIDTH, W_HEIGHT, 0.1f, 1000.f));
		cyborgShader.setMat4("view", camera.getViewMatrix());
		cyborgShader.setMat4("model", computeModelMatrix(glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, glm::vec3(1.0f, 0.0f, 0.0f)));
		cyborgShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		cyborgShader.setVec3("lightPos", dirLightPos);
		cyborgShader.setVec3("dirLight.position", dirLightPos);
		cyborgShader.setVec3("dirLight.ambient", glm::vec3(0.05f));
		cyborgShader.setVec3("dirLight.diffuse", glm::vec3(0.5f));
		cyborgShader.setVec3("dirLight.specular", glm::vec3(0.3f));
		cyborgShader.setInt("dirShadowMap", 4);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, depthTexture.id);

		cyborgShader.setFloat("material.shininess", 8.0f);
		cyborgShader.setVec3("viewPos", camera.getCameraPos());
		cyborg.Draw(cyborgShader);

		// checks events and swap buffers
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	glfwTerminate();

	return 0;
}