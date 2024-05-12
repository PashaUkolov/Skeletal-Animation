#pragma once 
#include "shader.h"
#include "glm/glm.hpp"
#include "camera.h"

class Renderer {
public:
	Renderer();
	static Renderer* getInstance();
	void initLine();
	void drawLine(glm::mat4 transform, glm::mat4 transform2, glm::vec3 color);
	Camera& getCamera() { return mCamera; };
	Shader& getShader() { return mShader; };
private:
	inline static Renderer* mInstance = nullptr;
	Camera mCamera;
	Shader mLineShader;
	Shader mShader;
};

