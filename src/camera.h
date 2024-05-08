#pragma once 

#include "glfw/glfw3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


static float zoomLevel = 0.0f;

const int WIDTH = 1000;
const int HEIGHT = 1000;

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

struct Camera {
    glm::mat4 transform;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;


    void updateCamera(float timer);
};