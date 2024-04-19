#include "stdio.h"
#include "glew/glew.h"
#include "glfw/glfw3.h"

#include "glm/glm.hpp"
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>

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

char* loadShader(const char* path) {
    FILE* file = fopen(path, "rb");
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = new char[size];
    size_t sz = fread(buffer, 1, size, file);
    fclose(file);

    buffer[size] = '\0';

    return buffer;
}

std::string parseShader(std::string filePath) {
    std::stringstream shaderCode;
    std::ifstream file{ filePath };
    std::string line;
    while (getline(file, line)) {
        shaderCode << line << "\n";
    }
    return shaderCode.str();
}

void checkErrors(unsigned int object, std::string type) {
    int success;
    char infoLog[2048];

    if (type != "PROGRAM") {
        glGetShaderiv(object, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(object, 2048, NULL, infoLog);
            std::cout << "ERROR::SHADER::compile time error Type:\n" << type
                << "\n" << infoLog << std::endl;
        }
    }
    else {
        glGetProgramiv(object, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(object, 2048, NULL, infoLog);
            std::cout << "ERROR::SHADER::link error Type:\n" << type
                << "\n" << infoLog << std::endl;
        }
    }
}

void compileShader(GLuint& vertexShader, GLuint& fragmentShader, GLuint& shaderID) {
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    auto vertexShaderCode = loadShader("../assets/shaders/default.vs");
    const char* vertexSource = vertexShaderCode;
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    checkErrors(vertexShader, "VERTEX");

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    auto fragmentShaderCode = loadShader("../assets/shaders/default.fs");
    const char* fragmentSource = fragmentShaderCode;
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    checkErrors(fragmentShader, "FRAGMENT");

    shaderID = glCreateProgram();
    glAttachShader(shaderID, vertexShader);
    glAttachShader(shaderID, fragmentShader);

    glLinkProgram(shaderID);
    checkErrors(shaderID, "PROGRAM");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
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

	/*float vertices[9] = {
		0.0f, 1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f
	};*/

    Model model = loadModel();

    /*for (auto& mesh : model.meshes) {
        for (auto& primitive : mesh.primitives) {
            for (auto& attribute : primitive.attributes) {
                if (attribute.first == "POSITION") {
                    auto a = attribute.second;
                }
            }
        }
    }*/

    std::vector<Vertex> vertices;

    for (size_t i = 0; i < model.meshes.size(); ++i) {
        Mesh& mesh = model.meshes[i];
        for (size_t j = 0; j < mesh.primitives.size(); ++j) {
            Primitive& primitive = mesh.primitives[j];
            // Access vertex data
            Accessor& accessor = model.accessors[primitive.attributes["POSITION"]];
            BufferView& bufferView = model.bufferViews[accessor.bufferView];
            Buffer& buffer = model.buffers[bufferView.buffer];

            // Extract actual vertex position values and store them in the custom vertex array
            const float* bufferData = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
            const int count = accessor.count;
            const int components = accessor.type == TINYGLTF_TYPE_VEC3 ? 3 : 2; // Assuming position is Vec3
            for (int k = 0; k < count; ++k) {
                Vertex vertex;
                for (int c = 0; c < components; ++c) {
                    vertex.position[c] = bufferData[k * components + c];
                }
                vertices.push_back(vertex);
            }
        }
    }

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint vertex, fragment;

    GLuint shader = glCreateProgram();
    compileShader(vertex, fragment, shader);

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);
        glBindVertexArray(vao);

        glDrawArrays(GL_TRIANGLES, 0, vertices.size());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}