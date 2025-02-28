#pragma once
#include <iostream>
#include <cassert>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class UniformBuffer
{
private:
	unsigned int bufferDataSize;
public:
	unsigned int UBO;

	// constructor will be used for buffer allocation
	UniformBuffer(unsigned int bufferDataSize, GLenum usage = GL_STATIC_DRAW) : bufferDataSize(bufferDataSize) {
		glGenBuffers(1, &UBO);
		glBindBuffer(GL_UNIFORM_BUFFER, UBO);
		glBufferData(GL_UNIFORM_BUFFER, bufferDataSize, NULL, usage);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	~UniformBuffer() {
		glDeleteBuffers(1, &UBO);
	}

	void bindBufferBase(unsigned int bindingPoint) {
		glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, UBO);
	}

	void setData(const void* data, unsigned int size, unsigned int offset = 0) const {
		assert(offset + size <= bufferDataSize);
		glBindBuffer(GL_UNIFORM_BUFFER, UBO);
		glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	unsigned int getBufferSize() const { return bufferDataSize; }
};