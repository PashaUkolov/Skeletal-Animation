#include "camera.h"

namespace {
    double gZoomLevel = 0.0f;
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    gZoomLevel += yoffset;
}

void Camera::updateCamera(const double timer) {
    transform = glm::mat4(1.0f);
    transform = glm::rotate(transform, glm::radians(static_cast<float>(timer)), glm::vec3(0.0f, 1.0f, 0.0f));
    viewMatrix = glm::mat4(1.0f);
    viewMatrix = glm::lookAt(glm::vec3(0.0f, 5.0f + gZoomLevel, -7.0f + -gZoomLevel), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0, 1, 0));
    projectionMatrix = glm::perspective(glm::radians(45.0f), static_cast<float>(WIDTH) / static_cast<float>(HEIGHT), 0.1f, 100.0f);
}