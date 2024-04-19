#pragma once
#include <string>
#include "glew/glew.h"

struct Shader {
	Shader(const std::string& path);
	GLuint shaderID;
private:
	void compileShader(const std::string& path);
};