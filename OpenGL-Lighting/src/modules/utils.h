#pragma once
#include <iostream>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../../stb/stb_image.h"
#include "texture.h"

enum class TextureColorSpace {
	Linear,
	sRGB
};

// adjusts the viewport when user resizes it
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

// process input in the renderer
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

unsigned int loadCubemap(std::vector<std::string> faces);
unsigned int createDefaultTexture();
unsigned int loadTexture(const char* path, bool flipVertically, TextureColorSpace space = TextureColorSpace::Linear);

// vertex array object references
unsigned int createCubeVAO();
unsigned int createSphereVAO(unsigned int& indicesCount, float radius = 1.0f, unsigned int sectorCount = 16, unsigned int stackCount = 16);
unsigned int createQuadVAO();
unsigned int createFrameVAO();

// model matrix helper
glm::mat4 computeModelMatrix(const glm::vec3& position, const glm::vec3& scale, float angleDegrees, const glm::vec3& rotationAxis);

void bindTextures(const std::vector<unsigned int>& textures, GLenum textureTarget = GL_TEXTURE_2D, unsigned int startUnit = GL_TEXTURE0);

glm::mat2x3 getTangentBitangentMatrix(glm::vec3 positions[3], glm::vec2 texCoords[3]);
glm::vec3 getVertexPosition(const float* vertices, int index);
glm::vec2 getUVPosition(const float* vertices, int index);