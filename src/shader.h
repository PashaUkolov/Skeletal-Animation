#pragma once
#include <string>
#include "glew/glew.h"

struct Shader {
	Shader() = default;
	explicit Shader(const std::string& path);
	GLuint id;
private:
	void compileShader(const std::string& path);
};