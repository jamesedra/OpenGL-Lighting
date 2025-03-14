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

int deferred_main()
{
	// initialization phase
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(W_WIDTH, W_HEIGHT, "Deferred Shading", NULL, NULL);
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

	// position color buffer
	Texture gPosition(W_WIDTH, W_HEIGHT, GL_RGBA16F, GL_RGBA);
	gPosition.setTexFilter(GL_NEAREST);
	gBuffer.attachTexture2D(gPosition, GL_COLOR_ATTACHMENT0);

	// normal color buffer
	Texture gNormal(W_WIDTH, W_HEIGHT, GL_RGBA16F, GL_RGBA);
	gNormal.setTexFilter(GL_NEAREST);
	gBuffer.attachTexture2D(gNormal, GL_COLOR_ATTACHMENT1);

	// specular color buffer
	Texture gAlbedoSpec(W_WIDTH, W_HEIGHT, GL_RGBA, GL_RGBA);
	gAlbedoSpec.setTexFilter(GL_NEAREST);
	gBuffer.attachTexture2D(gAlbedoSpec, GL_COLOR_ATTACHMENT2);

	// texture and renderbuffer attachments
	gBuffer.bind();
	unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);
	gBuffer.attachRenderbuffer(GL_DEPTH_STENCIL_ATTACHMENT, GL_DEPTH24_STENCIL8);
	
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

	// Shaders
	Shader gBufferShader("shaders/base_vertex.vert", "shaders/deferred/def_gbf.frag");
	Shader baseColorShader("shaders/post_process/framebuffer_quad.vert", "shaders/deferred/def_basecolor.frag");
	Shader lightingShader("shaders/deferred/def_lightvolume.vert", "shaders/deferred/def_lightvolume.frag");
	Shader lightSphereShader("shaders/base_vertex.vert", "shaders/emissive_color.frag");

	std::vector<unsigned int> textureIDs = { gPosition.id, gNormal.id, gAlbedoSpec.id };
	
	// Lighting data
	struct Light {
		glm::vec3 Position;
		float pad1;
		glm::vec3 Color;
		float Radius;
	};
	const unsigned int NR_LIGHTS = 32;
	Light lights[NR_LIGHTS];

	float radius = 5.0f;
	for (unsigned int i = 0; i < NR_LIGHTS; i++) {
		float angle = (float)i / (float)NR_LIGHTS * 2.0f * glm::pi<float>();
		lights[i].Position = glm::vec3(sin(angle) * radius, 0.5f, cos(angle) * radius);
		lights[i].Color = glm::vec3((sin(angle) + 1.0f) * 0.5f, (cos(angle) + 1.0f) * 0.5f, 0.5f);
		lights[i].Radius = radius;
	}
	UniformBuffer uboLights(sizeof(lights));
	unsigned int bindingPoint = 0;
	unsigned int uniformBlockIndex = glGetUniformBlockIndex(lightingShader.ID, "LightBlock");
	uboLights.bindBufferBase(bindingPoint);
	uboLights.setData(&lights, sizeof(lights));

	srand(glfwGetTime());
	// render loop
	while (!glfwWindowShouldClose(window))
	{
		// input
		processInput(window);

		// light movement test
		float time = glfwGetTime() * 0.5f;
		radius = 5.0f + sin(time) * (10.0f - 5.0f);
		for (unsigned int i = 0; i < NR_LIGHTS; i++) {
			float angle = (float)i / (float)NR_LIGHTS * 2.0f * glm::pi<float>();
			lights[i].Position = glm::vec3(sin(fmod(time * angle, 360.0)) * radius, 0.5f + (float)i / (float)NR_LIGHTS * (0.1f - 0.5f), cos(fmod(time * angle, 360.0)) * radius);
			lights[i].Color = glm::vec3((sin(fmod(time * angle, 360.0)) + 1.0f) * 0.5f, (cos(fmod(time * angle, 360.0)) + 1.0f) * 0.5f, 0.5f);
			lights[i].Radius = radius;
		}
		uboLights.setData(&lights, sizeof(lights));

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

		// Base color pass
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);

		baseColorShader.use();
		baseColorShader.setInt("gAlbedoSpec", 0);
		baseColorShader.setFloat("ambient", 0.5f);
		baseColorShader.setVec3("viewPos", camera.getCameraPos());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gAlbedoSpec.id);
		glBindVertexArray(frameVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// Lighting pass
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);

		lightingShader.use();
		lightingShader.setInt("gPosition", 0);
		lightingShader.setInt("gNormal", 1);
		lightingShader.setInt("gAlbedoSpec", 2);
		lightingShader.setVec3("viewPos", camera.getCameraPos());
		lightingShader.setMat4("projection", camera.getProjectionMatrix(W_WIDTH, W_HEIGHT, 0.1f, 1000.0f));
		lightingShader.setMat4("view", camera.getViewMatrix());
		bindTextures(textureIDs);

		for (unsigned int i = 0; i < NR_LIGHTS; i++) {
			model = glm::mat4(1.0f);
			model = glm::translate(model, lights[i].Position);
			model = glm::scale(model, glm::vec3(lights[i].Radius));
			lightingShader.setMat4("model", model);
			lightingShader.setVec3("Color", lights[i].Color);
			lightingShader.setInt("lightIndex", i);
			glBindVertexArray(lightSphere);
			glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, 0);
		}
		glDisable(GL_BLEND);
		glCullFace(GL_BACK);

		glEnable(GL_DEPTH_TEST);
		gBuffer.bind();
		glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.FBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, W_WIDTH, W_HEIGHT, 0, 0, W_WIDTH, W_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// additional forward rendering pass
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		lightSphereShader.use();
		lightSphereShader.setMat4("projection", camera.getProjectionMatrix(W_WIDTH, W_HEIGHT, 0.1f, 1000.0f));
		lightSphereShader.setMat4("view", camera.getViewMatrix());

		for (unsigned int i = 0; i < NR_LIGHTS; i++) {
			model = glm::mat4(1.0f);
			model = glm::translate(model, lights[i].Position);
			model = glm::scale(model, glm::vec3(0.25f));
			lightSphereShader.setMat4("model", model);
			lightSphereShader.setVec3("Color", lights[i].Color);
			glBindVertexArray(lightSphere);
			glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, 0);
		}
		glDisable(GL_BLEND);
		// checks events and swap buffers
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	glfwTerminate();

	return 0;
}