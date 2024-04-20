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

#include "shader.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tinygltf/tiny_gltf.h"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

const int WIDTH = 500;
const int HEIGHT = 500;

using namespace tinygltf;
Model loadModel(const std::string& path) {
    Model model;
    TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
    return model;
}

struct Vertex {
    float position[3];  // Position
    float normal[3];    // Normal (if available)
    float texcoord[2];  // Texture coordinates (if available)
};

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float getFrameTime() {
    double currentTime = glfwGetTime();
    deltaTime = currentTime - lastFrame;
    lastFrame = currentTime;
    return deltaTime;
}

void printMatrix(const glm::mat4& matrix) {
    system("cls");
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

    Shader shader("../assets/shaders/default");

    float timer = 0.0f;

    GLuint modelLoc = glGetUniformLocation(shader.id, "model");
    GLuint viewLoc = glGetUniformLocation(shader.id, "view");
    GLuint projectionLoc = glGetUniformLocation(shader.id, "projection");

    glm::mat4 modelMat = glm::mat4(1.0f);
    glm::mat4 viewMat = glm::mat4(1.0f);
    viewMat = glm::translate(viewMat, glm::vec3(0.0f, 0.0f, 50.0f));
    glm::mat4 projectionMat = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);

    glUniformMatrix4fv(modelLoc, 1, false, &(modelMat[0].x));
    glUniformMatrix4fv(viewLoc, 1, false, &(viewMat[0].x));
    glUniformMatrix4fv(projectionLoc, 1, false, &(projectionMat[0].x));

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    std::map<int, GLuint> vbos;
    using namespace tinygltf;
    Model model = loadModel("../assets/models/Mannequin.gltf");
    for (size_t i = 0; i < model.bufferViews.size(); i++) {
        BufferView bufferView = model.bufferViews[i];
        if (bufferView.target == 0) {
            continue;
        }
		GLuint modelVBO{};
        Buffer buffer = model.buffers[bufferView.buffer];
        glGenBuffers(1, &modelVBO);
        vbos[i] = modelVBO;
        glBindBuffer(bufferView.target, modelVBO);
        glBufferData(bufferView.target, bufferView.byteLength, &buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);
    }

    Scene& scene = model.scenes[model.defaultScene];
    auto mesh = model.meshes[0];
    for (size_t i = 0; i < mesh.primitives.size(); i++) {
        Primitive primitive = mesh.primitives[i];
        for (auto& attribute : primitive.attributes) {
			Accessor accessor = model.accessors[attribute.second];
            glBindBuffer(GL_ARRAY_BUFFER, vbos[accessor.bufferView]);
            if (attribute.first == "POSITION") {
                int size = accessor.type;
                bool isNormalized = accessor.normalized ? GL_TRUE : GL_FALSE;
                int stride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);

                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, size, GL_FLOAT, isNormalized, stride, BUFFER_OFFSET(accessor.byteOffset));
            }
        }
    }

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        timer += getFrameTime();

        glm::mat4 modelMat = glm::mat4(1.0f);
        //model = glm::translate(model, glm::vec3(glm::vec3(10.0f * sinf(timer) * 0.5f + 0.5f, 0.0f, 0.0f)));
        modelMat = glm::rotate(modelMat, glm::radians(timer * 50.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 view = glm::mat4(1.0f);
        float val = 100.0f * sinf(timer * 0.5) * 0.5f + 0.8f;
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -5.0f));
        float aspect = (float)WIDTH / (float)HEIGHT;
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

        glUniformMatrix4fv(modelLoc, 1, false, &(modelMat[0].x));
        glUniformMatrix4fv(viewLoc, 1, false, &(view[0].x));
        glUniformMatrix4fv(projectionLoc, 1, false, &(projection[0].x));

        glUseProgram(shader.id);

        for (int i = 0; i < mesh.primitives.size(); ++i) {
            Primitive primitive = mesh.primitives[i];
            Accessor accessor = model.accessors[primitive.indices];
            auto offset = BUFFER_OFFSET(accessor.byteOffset);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[accessor.bufferView]);
			glDrawElements(GL_TRIANGLES, accessor.count, accessor.componentType, BUFFER_OFFSET(accessor.byteOffset));
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}