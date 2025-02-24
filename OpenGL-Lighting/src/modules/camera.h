#pragma once

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Camera{
private:
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;
    float fov;

public:
    // Constructor with initialization list
    Camera(const glm::vec3& cameraPos, const glm::vec3& cameraFront, const glm::vec3& cameraUp, const float& fov);

    // Setter methods
    void setCameraPos(const glm::vec3& pos);
    void setCameraFront(const glm::vec3& front);
    void setCameraUp(const glm::vec3& up);
    void setFOV(const float fov);


    // Getter methods
    glm::vec3 getCameraPos() const;
    glm::vec3 getCameraFront() const;
    glm::vec3 getCameraUp() const;
    float getFOV() const;
};