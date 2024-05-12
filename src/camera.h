#pragma once 

#include "glfw/glfw3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

constexpr int WIDTH = 1080;
constexpr int HEIGHT = 720;

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

struct Camera {
    glm::mat4 transform;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;


    void updateCamera(const double timer);
};