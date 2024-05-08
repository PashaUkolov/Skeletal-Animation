#include "stdio.h"
#include "glew/glew.h"
#include "glfw/glfw3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <set>
#include <list>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "camera.h"
#include "animatedModel.h"

#include "renderer.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tinygltf/tiny_gltf.h"

float deltaTime = 0.0f;
float lastFrame = 0.0f;


float getFrameTime() {
    double currentTime = glfwGetTime();
    deltaTime = currentTime - lastFrame;
    lastFrame = currentTime;
    return deltaTime;
}

void printMatrix(const glm::mat4& matrix) {
    //system("cls");
    printf("MATRIX ========\n");
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            printf("%.1f ", matrix[i][j]);
        }
        printf("\n");
    }
}

int main(void) {
    if (!glfwInit()) {
        printf("unable to init glfw!\n");
    }

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "SkeletalAnimation", 0, 0);
    if (!window) {
        printf("ubable to create window\n");
        glfwTerminate();
    }

    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK) {
        printf("unable to init glew\n");
    }

    glfwSetScrollCallback(window, scrollCallback);

    Renderer* renderer = Renderer::getInstance();

    float timer = 0.0f;
    AnimatedModel animatedModel;

    while (!glfwWindowShouldClose(window)) {
        double frameBegin = glfwGetTime();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        timer += getFrameTime();
        
        renderer->getCamera().updateCamera(timer * 30.0f);

        animatedModel.draw();

        double frameEnd = glfwGetTime();
        char title[256];
        sprintf(title, "frame time: %.1f ms", (frameEnd - frameBegin) * 1000.0f);
        glfwSetWindowTitle(window, title);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}