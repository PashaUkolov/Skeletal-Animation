#include "stdio.h"
#include "glew/glew.h"
#include "glfw/glfw3.h"

#include "glm/glm.hpp"
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>

#include "shader.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tinygltf/tiny_gltf.h"

const int WIDTH = 500;
const int HEIGHT = 500;

using namespace tinygltf;
Model loadModel() {
    Model model;
    TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, "../assets/models/Mannequin.gltf");
    return model;
}

struct Vertex {
    float position[3];  // Position
    float normal[3];    // Normal (if available)
    float texcoord[2];  // Texture coordinates (if available)
};

int main(void) {
    if (!glfwInit()) {
        printf("unable to init glfw!\n");
    }

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "SOLAR", 0, 0);
    if (!window) {
        printf("ubable to create window\n");
        glfwTerminate();
    }

    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK) {
        printf("unable to init glew\n");
    }

	float vertices[9] = {
		0.0f, 1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f
	};

    Shader shader("../assets/shaders/default");

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader.shaderID);
        glBindVertexArray(vao);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}