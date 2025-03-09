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

int bloom_main()
{
	// initialization phase
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(W_WIDTH, W_HEIGHT, "Bloom Test", NULL, NULL);
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

	// HDR color attachment
	Texture colorBuffer(W_WIDTH, W_HEIGHT, GL_RGBA16F, GL_RGBA, GL_LINEAR, GL_CLAMP_TO_EDGE);
	Texture brightBuffer(W_WIDTH, W_HEIGHT, GL_RGBA16F, GL_RGBA, GL_LINEAR, GL_CLAMP_TO_EDGE);
	
	// HDR framebuffer
	Framebuffer tonemapper(W_WIDTH, W_HEIGHT);
	tonemapper.attachTexture2D(colorBuffer, GL_COLOR_ATTACHMENT0);
	tonemapper.attachTexture2D(brightBuffer, GL_COLOR_ATTACHMENT1);
	tonemapper.attachRenderbuffer(GL_DEPTH_STENCIL_ATTACHMENT, GL_DEPTH24_STENCIL8);
	tonemapper.bind();
	unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments);
	tonemapper.unbind();
	unsigned int FrameVAO = createFrameVAO();

	// Bloom buffers
	unsigned int pingpongFBO[2];
	unsigned int pingpongBuffer[2]; // color sttachments
	glGenFramebuffers(2, pingpongFBO);
	glGenTextures(2, pingpongBuffer);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
		glBindTexture(GL_TEXTURE_2D, pingpongBuffer[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, W_WIDTH, W_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongBuffer[i], 0);
	}

	// Directional shadows setup
	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	Texture depthTexture(SHADOW_WIDTH, SHADOW_HEIGHT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT);
	depthTexture.setTexFilter(GL_NEAREST);
	depthTexture.setTexWrap(GL_CLAMP_TO_BORDER);
	depthTexture.bind();
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	depthTexture.unbind();

	// Directional Shadow Framebuffer
	Framebuffer depthFBO(SHADOW_WIDTH, SHADOW_HEIGHT, depthTexture, GL_DEPTH_ATTACHMENT);
	depthFBO.bind();
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	depthFBO.unbind();

	// Shaders
	Shader depthDirShader("shaders/simple_depth.vert", "shaders/empty.frag");
	Shader floorShader("shaders/base_lit.vert", "shaders/base_lit_mrt.frag");
	Shader lightSourceShader("shaders/base_vertex.vert", "shaders/red.frag");
	Shader hdrShader("shaders/post_process/framebuffer_quad.vert", "shaders/post_process/rh_tonemapping.frag");
	Shader blurShader("shaders/post_process/framebuffer_quad.vert", "shaders/post_process/gaussian.frag");
	Shader bloomShader("shaders/post_process/framebuffer_quad.vert", "shaders/post_process/bloom.frag");

	// Floor setup
	unsigned int floorVAO = createQuadVAO();
	unsigned int tex_diff = loadTexture("resources/textures/bricks2.jpg", true, TextureColorSpace::sRGB);
	unsigned int tex_norm = loadTexture("resources/textures/bricks2_normal.jpg", true, TextureColorSpace::Linear);
	unsigned int tex_spec = createDefaultTexture();
	unsigned int tex_disp = loadTexture("resources/textures/bricks2_disp.jpg", true, TextureColorSpace::Linear);
	std::vector<unsigned int> textureIDs = { tex_diff, tex_spec, tex_norm, tex_disp, depthTexture.id };

	// Directional Light
	glm::vec3 dirLightPos(1.0f, 5.0f, 1.0f);
	float near_plane = 1.0f, far_plane = 17.5f;

	// Point Light
	unsigned int lightCubeVAO = createCubeVAO();
	glm::vec3 pointLightPos(0.0f, 0.5f, 0.0f);
	float constant = 1.0f;
	float linear = 0.14f;
	float quadratic = 0.7f;
	glm::vec3 diffuse = glm::vec3(12.0f);
	glm::vec3 specular = glm::vec3(1.0f);
	glm::vec3 ambient = glm::vec3(0.05f);

	// render loop
	while (!glfwWindowShouldClose(window))
	{
		// input
		processInput(window);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
		glm::mat4 lightView = glm::lookAt(dirLightPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 lightSpaceMatrix = lightProjection * lightView;

		// get directional shadow pass
		glCullFace(GL_FRONT);
		glEnable(GL_DEPTH_TEST); // enable depth testing (disabled for tone mapping)

		depthDirShader.use();
		depthDirShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		depthDirShader.setMat4("model", computeModelMatrix(glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(10.0f, 10.0f, 10.0f), -90.0f, glm::vec3(1.0f, 0.0f, 0.0f)));
		depthFBO.bind();
		glClear(GL_DEPTH_BUFFER_BIT);
		glBindVertexArray(floorVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, pointLightPos);
		model = glm::rotate(model, glm::radians((float)std::fmod(glfwGetTime() * 50, 360.0)), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.5, 0.5, 0.5));
		depthDirShader.setMat4("model", model);

		glBindVertexArray(lightCubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		depthFBO.unbind();
		glCullFace(GL_BACK);

		// saved rendered scene to the tonemapper
		tonemapper.bind();
		glViewport(0, 0, W_WIDTH, W_HEIGHT);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		lightSourceShader.use();
		lightSourceShader.setMat4("projection", camera.getProjectionMatrix(W_WIDTH, W_HEIGHT, 0.1f, 1000.f));
		lightSourceShader.setMat4("view", camera.getViewMatrix());
		model = glm::mat4(1.0f);
		model = glm::translate(model, pointLightPos);
		model = glm::rotate(model, glm::radians((float)std::fmod(glfwGetTime() * 50, 360.0)), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.5, 0.5, 0.5));
		lightSourceShader.setMat4("model", model);
		glBindVertexArray(lightCubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		floorShader.use();
		floorShader.setMat4("projection", camera.getProjectionMatrix(W_WIDTH, W_HEIGHT, 0.1f, 1000.f));
		floorShader.setMat4("view", camera.getViewMatrix());
		floorShader.setMat4("model", computeModelMatrix(glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(10.0f, 10.0f, 10.0f), -90.0f, glm::vec3(1.0f, 0.0f, 0.0f)));
		floorShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		floorShader.setVec3("lightPos", dirLightPos);
		floorShader.setVec3("dirLight.position", dirLightPos);
		floorShader.setVec3("dirLight.ambient", glm::vec3(0.05f));
		floorShader.setVec3("dirLight.diffuse", glm::vec3(0.2f));
		floorShader.setVec3("dirLight.specular", glm::vec3(0.3f));

		floorShader.setVec4("pointLight.positionAndConstant", glm::vec4(pointLightPos, constant));
		floorShader.setVec4("pointLight.ambientAndLinear", glm::vec4(ambient, linear));
		floorShader.setVec4("pointLight.diffuseAndQuadratic", glm::vec4(diffuse, quadratic));
		floorShader.setVec4("pointLight.specular", glm::vec4(specular, 1.0));

		floorShader.setInt("material.diffuse", 0);
		floorShader.setInt("material.specular", 1);
		floorShader.setInt("material.normal", 2);
		floorShader.setInt("material.depth", 3);
		floorShader.setInt("dirShadowMap", 4);

		floorShader.setFloat("material.shininess", 32.0f);
		floorShader.setVec3("viewPos", camera.getCameraPos());
		floorShader.setFloat("height_scale", 0.05f);

		bindTextures(textureIDs);
		glBindVertexArray(floorVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		tonemapper.unbind();

		// blur pass
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		bool horizontal = true, first_iteration = true;
		int amount = 10;
		blurShader.use();
		for (unsigned int i = 0; i < amount; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
			blurShader.setInt("horizontal", horizontal);
			blurShader.setInt("image", 0);
			glBindVertexArray(FrameVAO);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, first_iteration ? brightBuffer.id : pingpongBuffer[!horizontal]);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			horizontal = !horizontal;
			if (first_iteration) first_iteration = false;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		bloomShader.use();
		bloomShader.setInt("sceneBuffer", 0);
		bloomShader.setInt("blurBuffer", 1);
		bloomShader.setFloat("exposure", 0.05);
		glBindVertexArray(FrameVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, colorBuffer.id);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, pingpongBuffer[1]);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		
		// checks events and swap buffers
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	glfwTerminate();

	return 0;
}