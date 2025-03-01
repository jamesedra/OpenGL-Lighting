#include "Camera.h"

// Constructor implementation
Camera::Camera(const glm::vec3& cameraPos, const glm::vec3& cameraFront, const glm::vec3& cameraUp, const float& fov)
    : cameraPos(cameraPos), cameraFront(cameraFront), cameraUp(cameraUp), fov(fov)
{
}

// Setter implementations
void Camera::setCameraPos(const glm::vec3& pos)
{
    cameraPos = pos;
}

void Camera::setCameraFront(const glm::vec3& front)
{
    cameraFront = front;
}

void Camera::setCameraUp(const glm::vec3& up)
{
    cameraUp = up;
}

void Camera::setFOV(const float fov)
{
    this->fov = fov;
}

// Getter implementations
glm::mat4 Camera::getProjectionMatrix(float width, float height, float near, float far)
{
    return glm::perspective(glm::radians(fov), width / height, near, far);
}

glm::mat4 Camera::getViewMatrix()
{
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    return view;
}

glm::vec3 Camera::getCameraPos() const
{
    return cameraPos;
}

glm::vec3 Camera::getCameraFront() const
{
    return cameraFront;
}

glm::vec3 Camera::getCameraUp() const
{
    return cameraUp;
}

float Camera::getFOV() const
{
    return fov;
}
