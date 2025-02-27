#pragma once

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


class Shader
{
public:
	// program ID
	unsigned int ID;

	// vert and frag shader constructor. Paths should start at root directory.
	Shader(const char* vertexPath, const char* fragmentPath);

	// vert, geom, and frag shader constructor
	Shader(const char* vertexPath, const char* geometryPath, const char* fragmentPath);

	// use and activate the shader
	void use();

	// utility uniform functions
	void setBool(const std::string& name, bool value) const;
	void setInt(const std::string& name, int value) const;
	void setFloat(const std::string& name, float value) const;
	void setVec2(const std::string& name, const glm::vec2& vec) const;
	void setVec3(const std::string& name, float x, float y, float z) const;
	void setVec3(const std::string& name, const glm::vec3& vec) const;
	void setMat4(const std::string& name, const glm::mat4& mat) const;
};