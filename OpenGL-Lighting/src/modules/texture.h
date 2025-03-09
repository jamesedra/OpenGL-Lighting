#pragma once

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>

class Texture {
public:
	unsigned int id;
	int width, height;

	static const std::unordered_map<GLenum, GLenum> sizedFormatToType;
	static const std::unordered_map<GLenum, GLenum> unsizedFormatToType;

	// base constructor
	Texture(int width, int height, GLenum internalFormat, GLenum baseFormat, const GLvoid* data = NULL) : width(width), height(height) {
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		GLenum type = getDataType(internalFormat);
		if (type != -1)
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, baseFormat, type, data);
		else
			std::cerr << "Error: baseFormat not supported." << std::endl;
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// overloaded constructor if they can provide the filter and wrap
	Texture(int width, int height, GLenum internalFormat, GLenum baseFormat, GLint filter, GLint wrap, const GLvoid* data = NULL) : width(width), height(height) {
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		GLenum type = getDataType(internalFormat);
		if (type != -1)
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, baseFormat, type, data);
		else
			std::cerr << "Error: baseFormat not supported." << std::endl;
		setTexFilter(filter);
		setTexWrap(wrap);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void setTexFilter(GLint filter) {
		glBindTexture(GL_TEXTURE_2D, id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
		unbind();
	}

	void setTexWrap(GLint wrap) {
		glBindTexture(GL_TEXTURE_2D, id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
		unbind();
	}

	void bind() {
		glBindTexture(GL_TEXTURE_2D, id);
	}

	void unbind() {
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void genMipMap() {
		bind();
		glGenerateMipmap(GL_TEXTURE_2D);
	}

private:
	GLenum getDataType(GLenum format)
	{
		auto it = sizedFormatToType.find(format);
		if (it != sizedFormatToType.end()) return it->second;
		it = unsizedFormatToType.find(format);
		if (it != unsizedFormatToType.end()) return it->second;

		return -1; // invalid return
	}
};

