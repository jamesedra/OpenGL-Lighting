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

	// G-Buffer
	Framebuffer gBuffer(W_WIDTH, W_HEIGHT);
	Texture gPosition(W_WIDTH, W_HEIGHT, GL_RGBA16F, GL_RGBA, GL_NEAREST, GL_CLAMP_TO_EDGE);
	gBuffer.attachTexture2D(gPosition, GL_COLOR_ATTACHMENT0);
	Texture gNormal(W_WIDTH, W_HEIGHT, GL_RGBA16F, GL_RGBA);
	gNormal.setTexFilter(GL_NEAREST);
	gBuffer.attachTexture2D(gNormal, GL_COLOR_ATTACHMENT1);
	Texture gAlbedoSpec(W_WIDTH, W_HEIGHT, GL_RGBA, GL_RGBA);
	gAlbedoSpec.setTexFilter(GL_NEAREST);
	gBuffer.attachTexture2D(gAlbedoSpec, GL_COLOR_ATTACHMENT2);
	// texture and renderbuffer attachments
	gBuffer.bind();
	unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);
	gBuffer.attachRenderbuffer(GL_DEPTH_STENCIL_ATTACHMENT, GL_DEPTH24_STENCIL8);

	// SSAO buffer
	Framebuffer ssaoBuffer(W_WIDTH, W_HEIGHT);
	Texture ssaoColor(W_WIDTH, W_HEIGHT, GL_RED, GL_RED);
	ssaoColor.setTexFilter(GL_NEAREST);
	ssaoBuffer.attachTexture2D(ssaoColor, GL_COLOR_ATTACHMENT0);

	// SSAO blur framebuffer
	Framebuffer ssaoBlurBuffer(W_WIDTH, W_HEIGHT);
	Texture ssaoBlurColor(W_WIDTH, W_HEIGHT, GL_RED, GL_RED);
	ssaoBlurColor.setTexFilter(GL_NEAREST);
	ssaoBlurBuffer.attachTexture2D(ssaoBlurColor, GL_COLOR_ATTACHMENT0);

	//stbi_set_flip_vertically_on_load(true); // set if needed

	// output frame
	unsigned int frameVAO = createFrameVAO();

	// Objects
	Model cyborg("resources/objects/cyborg/cyborg.obj");
	unsigned int floorVAO = createQuadVAO();
	unsigned int tex_diff = loadTexture("resources/textures/brickwall.jpg", true, TextureColorSpace::sRGB);
	unsigned int tex_spec = createDefaultTexture();
	unsigned int indicesCount;
	unsigned int lightSphere = createSphereVAO(indicesCount, 1.0f, 16, 16);

	// Normal oriented hemisphere
	std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
	std::default_random_engine generator;
	std::vector<glm::vec3> ssaoKernel;

	for (unsigned int i = 0; i < 64; ++i)
	{
		glm::vec3 sample(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) // z-axis only range 0 to 1
		);
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);

		// distribute samples nearer to the fragment
		float scale = (float)i / 64.0;
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		ssaoKernel.push_back(sample);
	}

	// Random rotation vector texture
	std::vector<glm::vec3> ssaoNoise;
	for (unsigned int i = 0; i < 16; i++)
	{
		glm::vec3 noise(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			0.0f
		);
		ssaoNoise.push_back(noise);
	}
	Texture noiseTexture(4, 4, GL_RGBA16F, GL_RGB, GL_NEAREST, GL_REPEAT, &ssaoNoise[0]);

	// Shaders
	Shader gBufferShader("shaders/deferred/def_gbf_ssao.vert", "shaders/deferred/def_gbf_ssao.frag");
	Shader ssaoShader("shaders/post_process/framebuffer_quad.vert", "shaders/deferred/def_ssao.frag");
	Shader ssaoBlurShader("shaders/post_process/framebuffer_quad.vert", "shaders/deferred/def_ssao_blur.frag");
	Shader baseColorShader("shaders/post_process/framebuffer_quad.vert", "shaders/deferred/def_basecolor.frag");
	Shader lightingShader("shaders/post_process/framebuffer_quad.vert", "shaders/deferred/def_lit_ao.frag");
	Shader lightSphereShader("shaders/base_vertex.vert", "shaders/emissive_color.frag");

	std::vector<unsigned int> gBufferTex = { gPosition.id, gNormal.id, gAlbedoSpec.id };
	std::vector<unsigned int> aoBufferTex = { gPosition.id, gNormal.id, noiseTexture.id };
	std::vector<unsigned int> lightingPassTex = { gPosition.id, gNormal.id, gAlbedoSpec.id, ssaoBlurColor.id };

	srand(glfwGetTime());
	// render loop
	while (!glfwWindowShouldClose(window))
	{
		// input
		processInput(window);

		// Geometry pass
		gBuffer.bind();
		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		gBufferShader.use();
		gBufferShader.setMat4("projection", camera.getProjectionMatrix(W_WIDTH, W_HEIGHT, 0.1f, 1000.0f));
		gBufferShader.setMat4("view", camera.getViewMatrix());

		// render cyborg model
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		gBufferShader.setMat4("model", model);
		cyborg.Draw(gBufferShader);

		// render floor
		gBufferShader.setMat4("model", computeModelMatrix(glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(10.0f, 10.0f, 10.0f), -90.0f, glm::vec3(1.0f, 0.0f, 0.0f)));
		gBufferShader.setInt("texture_diffuse1", 0);
		gBufferShader.setInt("texture_specular1", 1);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex_diff);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex_spec);
		glBindVertexArray(floorVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		gBuffer.unbind();

		// SSAO pass
		ssaoBuffer.bind();
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		ssaoShader.use();
		ssaoShader.setMat4("projection", camera.getProjectionMatrix(W_WIDTH, W_HEIGHT, 0.1f, 1000.0f));
		ssaoShader.setInt("gPosition", 0);
		ssaoShader.setInt("gNormal", 1);
		ssaoShader.setInt("texNoise", 2);
		// send kernel samples to shader
		for (unsigned int i = 0; i < 64; i++) {
			ssaoShader.setVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
		}
		// render quad
		bindTextures(aoBufferTex);
		glBindVertexArray(frameVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		ssaoBuffer.unbind();

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// SSAO blur pass
		ssaoBlurBuffer.bind();
		ssaoBlurShader.use();
		ssaoBlurShader.setInt("ssaoInput", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ssaoColor.id);
		glBindVertexArray(frameVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		ssaoBlurBuffer.unbind(); 

		// Lighting pass
		lightingShader.use();
		// input matrices
		lightingShader.setMat4("projection", camera.getProjectionMatrix(W_WIDTH, W_HEIGHT, 0.1f, 1000.0f));
		lightingShader.setMat4("view", camera.getViewMatrix());
		// input passes
		lightingShader.setInt("gPosition", 0);
		lightingShader.setInt("gNormal", 1);
		lightingShader.setInt("gAlbedoSpec", 2);
		lightingShader.setInt("ssao", 3);
		// light uniforms
		lightingShader.setVec3("light.Position", glm::vec3(1.0f));
		lightingShader.setVec3("light.Color", glm::vec3(1.0f));
		lightingShader.setFloat("light.Linear", 0.0014f);
		lightingShader.setFloat("light.Quadratic", 0.07f);
		lightingShader.setFloat("light.Radius", 1.0f);

		bindTextures(lightingPassTex);
		glBindVertexArray(frameVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glEnable(GL_DEPTH_TEST);
		gBuffer.bind();
		glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.FBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, W_WIDTH, W_HEIGHT, 0, 0, W_WIDTH, W_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// checks events and swap buffers
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	glfwTerminate();

	return 0;
}