#include "shader.h"

Shader::Shader(const std::string& path) {
    GLuint shader = glCreateProgram();
    compileShader(path);
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

void checkErrors(unsigned int object, std::string type) {
    int success;
    char infoLog[2048];

    if (type != "PROGRAM") {
        glGetShaderiv(object, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(object, 2048, NULL, infoLog);
            printf("ERROR::SHADER::compile time error Type: %s, %s\n", type, infoLog);
        }
    }
    else {
        glGetProgramiv(object, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(object, 2048, NULL, infoLog);
            printf("ERROR::SHADER::link error Type: %s, \n", type, infoLog);
        }
    }
}

void Shader::compileShader(const std::string& path) {
    GLuint vertexShader, fragmentShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    std::string vsPath = path + std::string(".vs");
    auto vertexShaderCode = loadShader(vsPath.c_str());
    const char* vertexSource = vertexShaderCode;
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    checkErrors(vertexShader, "VERTEX");

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    std::string fsPath = path + std::string(".fs");
    auto fragmentShaderCode = loadShader(fsPath.c_str());
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
