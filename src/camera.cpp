#include "camera.h"

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    zoomLevel += yoffset;
}

void Camera::updateCamera(float timer) {
    transform = glm::mat4(1.0f);
    transform = glm::rotate(transform, glm::radians(timer), glm::vec3(0.0f, 1.0f, 0.0f));
    viewMatrix = glm::mat4(1.0f);
    viewMatrix = glm::lookAt(glm::vec3(0.0f, 5.0f + zoomLevel, -7.0f + -zoomLevel), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0, 1, 0));
    projectionMatrix = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
}