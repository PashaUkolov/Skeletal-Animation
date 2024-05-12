#include "shader.h"

Shader::Shader(const std::string& path) {
    compileShader(path);
}

char* loadShader(const char* path) {
    FILE* file = fopen(path, "rb");
    (void)fseek(file, 0, SEEK_END);
    const long size = ftell(file);
    (void)fseek(file, 0, SEEK_SET);

    char* buffer = new char[size];
    size_t sz = fread(buffer, 1, size, file);
    (void)fclose(file);

    buffer[size] = '\0';

    return buffer;
}

void checkErrors(const unsigned int object, const std::string& type) {
    int success;
    char infoLog[2048];

    if (type != "PROGRAM") {
        glGetShaderiv(object, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(object, 2048, nullptr, infoLog);
            printf("ERROR::SHADER::compile time error Type: %s, %s\n", type.c_str(), infoLog);
        }
    }
    else {
        glGetProgramiv(object, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(object, 2048, nullptr, infoLog);
            printf("ERROR::SHADER::link error Type: %s, %s \n", type.c_str(), infoLog);
        }
    }
}

void Shader::compileShader(const std::string& path) {
    const GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const std::string vsPath = path + std::string(".vs");
    const char* vertexSource = loadShader(vsPath.c_str());
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);
    checkErrors(vertexShader, "VERTEX");

    const GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const std::string fsPath = path + std::string(".fs");
    const char* fragmentSource = loadShader(fsPath.c_str());
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);
    checkErrors(fragmentShader, "FRAGMENT");

    id = glCreateProgram();
    glAttachShader(id, vertexShader);
    glAttachShader(id, fragmentShader);

    glLinkProgram(id);
    checkErrors(id, "PROGRAM");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}
