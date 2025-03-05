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
	glEnable(GL_CULL_FACE);
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
	Shader depthDirShader("shaders/simple_depth.vert", "shaders/empty.frag");
	Shader depthPointShader("shaders/simple_depth.vert", "shaders/simple_depth.geom", "shaders/linear_depth.frag");
	Shader ppShader("shaders/framebuffer_quad.vert", "shaders/simple_depth.frag");


	// point lights
	UniformBuffer uboPointLights(sizeof(PointLightsBlock), GL_STATIC_DRAW);

	unsigned int bindingPoint = 0;
	unsigned int uniformBlockIndex = glGetUniformBlockIndex(shader.ID, "PointLights");
	glUniformBlockBinding(shader.ID, uniformBlockIndex, bindingPoint);
	uboPointLights.bindBufferBase(bindingPoint);

	glm::vec3 pointLightPositions[] = {
		glm::vec3(2.0f, 1.5f, 2.25f),
		// glm::vec3(5.0f, 1.5f, 5.0f),
		// glm::vec3(-5.0f, 1.5f, 5.0f),
		// glm::vec3(5.0f, 1.5f, -5.0f),
	};

	std::vector<PointLightData> pointLights;
	float constant = 1.0f;
	float linear = 0.2f;
	float quadratic = 0.22f;
	glm::vec3 diffuse = glm::vec3(0.7f);
	glm::vec3 specular = glm::vec3(1.0f);
	glm::vec3 ambient = glm::vec3(0.2f);

	for (int i = 0; i < sizeof(pointLightPositions) / sizeof(glm::vec3); i++) {
		pointLights.push_back({ glm::vec4(pointLightPositions[i], constant), glm::vec4(ambient, linear), glm::vec4(diffuse, quadratic), glm::vec4(specular, 1.0) });
	}

	PointLightsBlock lightsBlock(pointLights);
	uboPointLights.setData(&lightsBlock, sizeof(PointLightsBlock));
	
	Model object("resources/objects/backpack/backpack.obj");

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

	unsigned int depthCubeFBO;
	glGenFramebuffers(1, &depthCubeFBO);
	// point shadow mapping
	unsigned int depthCubemap;
	glGenTextures(1, &depthCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
	for (unsigned int i = 0; i < 6; ++i) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	
	glBindFramebuffer(GL_FRAMEBUFFER, depthCubeFBO);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// std::cout << "depthCubemap = " << depthCubemap << std::endl;
	// glBindTexture(GL_TEXTURE_CUBE_MAP, 0); 
	// for some reason, unbinding the cube does not produce shadow lights. I tried checking collisions between overriding texture ids but there's nothing. :(


	glm::vec3 lightPos(2.0f, 1.5f, 2.25f);
	//glm::vec3 lightPos(-2.0f, 4.0f, -1.0f);
	float near = 1.0f;
	float far = 25.0f;
	glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near, far);

	// prep view matrix for each cube face
	std::vector<glm::mat4> shadowTransforms;
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0))); // +X
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0,0.0, 0.0), glm::vec3(0.0, -1.0, 0.0))); // -X
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0,  1.0))); // +Y
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0,-1.0, 0.0), glm::vec3(0.0, 0.0, -1.0))); // -Y
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0))); // +Z
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0,-1.0), glm::vec3(0.0, -1.0, 0.0))); // -Z


	unsigned int frame = createFrameVAO();
	ppShader.use();
	ppShader.setInt("depthMap", 0);

	// prepare to bind textures
	std::vector<unsigned int> textureIDs = { tex_diff, tex_spec, depthTexture.id };

	// render loop
	while (!glfwWindowShouldClose(window))
	{
		// input
		processInput(window);

		// render commands

		// first pass: render to depth map (directional shadows)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/*
		float near_plane = 1.0f, far_plane = 27.5f;
		glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
		glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 lightSpaceMatrix = lightProjection * lightView;

		glCullFace(GL_FRONT);
		depthDirShader.use();
		depthDirShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		depthDirShader.setMat4("model", computeModelMatrix(glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(10.0f, 5.0f, 10.0f), 90.0f, glm::vec3(1.0f, 0.0f, 0.0f)));

		depthFBO.bind();
		glClear(GL_DEPTH_BUFFER_BIT);
		glBindVertexArray(floorVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		depthDirShader.setMat4("model", computeModelMatrix(glm::vec3(0.0f, 2.5f, 0.0f),
			glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, glm::vec3(1.0f, 0.0f, 0.0f)));
		object.Draw(depthDirShader);
		depthFBO.unbind();
		glCullFace(GL_BACK);
		*/

		// render depth map (point shadows)
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthCubeFBO);
		glClear(GL_DEPTH_BUFFER_BIT);

		depthPointShader.use();
		depthPointShader.setMat4("lightSpaceMatrix", glm::mat4(1.0f)); // no need for light space calculations
		depthPointShader.setVec3("lightPos", lightPos);
		for (unsigned int i = 0; i < 6; ++i) {
			depthPointShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
		}
		depthPointShader.setFloat("far_plane", far);

		glBindVertexArray(floorVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		depthPointShader.setMat4("model", computeModelMatrix(glm::vec3(0.0f, 2.5f, 0.0f),
			glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, glm::vec3(1.0f, 0.0f, 0.0f)));
		object.Draw(depthPointShader);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glViewport(0, 0, W_WIDTH, W_HEIGHT);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/* for depth debugging only
		ppShader.use();
		ppShader.setFloat("near_plane", near_plane);
		ppShader.setFloat("far_plane", far_plane);
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
		// shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		shader.setMat4("lightSpaceMatrix", glm::mat4(1.0f));

		shader.setVec3("dirLight.direction", glm::normalize(-lightPos));
		shader.setVec3("dirLight.position", lightPos);
		shader.setVec3("dirLight.ambient", glm::vec3(0.05f));
		shader.setVec3("dirLight.diffuse", glm::vec3(1.0f));
		shader.setVec3("dirLight.specular", glm::vec3(1.0f));

		shader.setInt("material.diffuse", 0);
		shader.setInt("material.specular", 1);
		shader.setInt("shadowMap", 2);
		
		shader.setFloat("material.shininess", 32.0f);

		shader.setVec3("viewPos", camera.getCameraPos());

		shader.setFloat("far_plane", far);
		bindTextures(textureIDs);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
		shader.setInt("shadowCubeMap", 3);

		glBindVertexArray(floorVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		shader.setMat4("model", computeModelMatrix(glm::vec3(0.0f, 1.7f, 0.0f),
			glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, glm::vec3(1.0f, 0.0f, 0.0f)));
		object.Draw(shader);

		// checks events and swap buffers
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	glfwTerminate();

	return 0;
}

