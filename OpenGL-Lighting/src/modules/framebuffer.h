#pragma once
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "texture.h"

class Framebuffer
{
private:
	unsigned int texture;
	unsigned int rbo;
	int width, height;
	int samples = 4;

public:
	unsigned int FBO;

	Framebuffer(int width, int height) : width(width), height(height) {
		glGenFramebuffers(1, &FBO);
		unbind();
	}

	Framebuffer(int width, int height, const Texture& tex, GLenum attachment) : width(width), height(height) {
		glGenFramebuffers(1, &FBO);
		attachTexture2D(tex, attachment);
	}

	void attachTexture2D(const Texture& tex, GLenum attachment) {
		bind();
		texture = tex.id;
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture, 0);
		unbind();
	}

	void attachTexture(GLenum attachment, unsigned int textureID) {
		bind();
		glFramebufferTexture(GL_FRAMEBUFFER, attachment, textureID, 0);
		unbind();
	}

	void attachRenderbuffer(GLenum attachment, GLenum internalFormat) {
		bind();
		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		unbind();
	}

	void editRenderbufferStorage(int width, int height, GLenum internalFormat) {
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		this->width = width;
		this->height = height;
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, width, height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		unbind();
	}

	bool isComplete() {
		bind();
		bool complete = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
		unbind();
		return complete;
	}

	// to be refactored
	Framebuffer(int width, int height, int samples) : width(width), height(height), samples(samples)
	{
		glGenFramebuffers(1, &FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);

		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, width, height, GL_TRUE);
		glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, texture, 0);
		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete." << std::endl;

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glViewport(0, 0, width, height);
	}

	void unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	unsigned int getTexture() const 
	{ 
		return texture; 
	}

	unsigned int getRBO() const
	{
		return rbo;
	}

	~Framebuffer()
	{
		glDeleteFramebuffers(1, &FBO);
		glDeleteTextures(1, &texture);
		glDeleteRenderbuffers(1, &rbo);
	}
};