#pragma once
#include <iostream>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../stb/stb_image.h"

// adjusts the viewport when user resizes it
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

// process input in the renderer
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

unsigned int loadCubemap(std::vector<std::string> faces);
unsigned int createDefaultTexture();
unsigned int loadTexture(const char* path, bool flipVertically);

// vertex array object references
unsigned int createCubeVAO();
unsigned int createQuadVAO();
unsigned int createFrameVAO();