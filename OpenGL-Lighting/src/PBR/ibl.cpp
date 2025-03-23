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

	GLFWwindow* window = glfwCreateWindow(W_WIDTH, W_HEIGHT, "PBR IBL", NULL, NULL);
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
	//glFrontFace(GL_CCW);
	//glCullFace(GL_BACK);
	// glEnable(GL_CULL_FACE);
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

	// Shaders
	Shader PBRShader("shaders/base_lit.vert", "shaders/pbr/pbr_textures_wnormals.frag");
	Shader EQRToCubemap("shaders/cubemapping/eqr_to_cubemap.vert", "shaders/cubemapping/eqr_to_cubemap.frag");
	Shader Skybox("shaders/cubemapping/skybox.vert", "shaders/cubemapping/skybox.frag");
	Shader IrradianceShader("shaders/cubemapping/eqr_to_cubemap.vert", "shaders/cubemapping/irradiance_convolution.frag");

	// Objects
	unsigned int indicesCount;
	unsigned int sphere = createSphereVAO(indicesCount, 1.0f, 64, 64);
	unsigned int cube = createCubeVAO();

	// Textures
	unsigned int tex_albedo = loadTexture("resources/textures/pbr/rusted_iron/albedo.png", true, TextureColorSpace::sRGB);
	unsigned int tex_normal = loadTexture("resources/textures/pbr/rusted_iron/normal.png", true, TextureColorSpace::Linear);
	unsigned int tex_metallic = loadTexture("resources/textures/pbr/rusted_iron/metallic.png", true, TextureColorSpace::Linear);
	unsigned int tex_roughness = loadTexture("resources/textures/pbr/rusted_iron/roughness.png", true, TextureColorSpace::Linear);
	unsigned int tex_ao = loadTexture("resources/textures/pbr/rusted_iron/ao.png", true, TextureColorSpace::Linear);

	// HDR
	Framebuffer hdrCapture(512, 512);
	hdrCapture.attachRenderbuffer(GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT24);

	unsigned int hdr = loadHDR("resources/textures/hdr/newport_loft.hdr", true);

	unsigned int envCubemap;
	glGenTextures(1, &envCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] =
	{
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	EQRToCubemap.use();
	EQRToCubemap.setInt("equirectangularMap", 0);
	EQRToCubemap.setMat4("projection", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hdr);

	hdrCapture.bind();
	for (unsigned int i = 0; i < 6; i++)
	{
		EQRToCubemap.setMat4("view", captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(cube);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
	}
	hdrCapture.unbind();

	// Irradiance
	unsigned int irradianceCubemap;
	glGenTextures(1, &irradianceCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceCubemap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		// no need for high resolution due to low frequency detailing
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	hdrCapture.editRenderbufferStorage(32, 32, GL_DEPTH_COMPONENT24);

	IrradianceShader.use();
	IrradianceShader.setInt("environmentMap", 0);
	IrradianceShader.setMat4("projection", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	hdrCapture.bind();
	for (unsigned int i = 0; i < 6; i++)
	{
		IrradianceShader.setMat4("view", captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceCubemap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(cube);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
	}
	hdrCapture.unbind();

	// HDR pre-filtered map
	unsigned int prefilterMap;
	glGenTextures(1, &prefilterMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // enable trilinear filtering
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	// Static uniforms for skyboxes
	Skybox.use();
	Skybox.setMat4("projection", camera.getProjectionMatrix(W_WIDTH, W_HEIGHT, 0.1f, 1000.0f));
	Skybox.setInt("environmentMap", 0);

	// Lighting
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

	std::vector<unsigned int> sphereTex = { tex_albedo, tex_normal, tex_metallic, tex_roughness, tex_ao, };

	glViewport(0, 0, W_WIDTH, W_HEIGHT);
	// render loop
	while (!glfwWindowShouldClose(window))
	{
		// input
		processInput(window);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// PBR Sphere
		PBRShader.use();
		PBRShader.setMat4("projection", camera.getProjectionMatrix(W_WIDTH, W_HEIGHT, 0.1f, 1000.0f));
		PBRShader.setMat4("view", camera.getViewMatrix());

		float time = glfwGetTime();
		PBRShader.setMat4("model", computeModelMatrix(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), fmod(time * 30.0f, 360.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
		PBRShader.setVec3("viewPos", camera.getCameraPos());

		// material uniforms, flip commented out code if not using textures
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
		PBRShader.setInt("irradianceMap", 5);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceCubemap);

		// light uniforms
		for (int i = 0; i < 4; ++i)
		{
			PBRShader.setVec3("lights[" + std::to_string(i) + "].position", lightPositions[i]);
			PBRShader.setVec3("lights[" + std::to_string(i) + "].color", lightColors[i]);
		}
		glBindVertexArray(sphere);
		glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, 0);

		// Skybox
		glFrontFace(GL_CW);
		glDepthFunc(GL_LEQUAL);
		Skybox.use();
		glm::mat4 view = glm::mat4(1.0f);
		glm::vec3 cameraPos = camera.getCameraPos();
		view = glm::lookAt(cameraPos, cameraPos + camera.getCameraFront(), camera.getCameraUp());
		Skybox.setMat4("view", glm::mat4(glm::mat3(view)));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
		//glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceCubemap);
		glBindVertexArray(cube);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glDepthFunc(GL_LESS);
		glFrontFace(GL_CCW);

		// checks events and swap buffers
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	glfwTerminate();

	return 0;
}