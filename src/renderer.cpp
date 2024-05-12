#include "renderer.h"
#include "shader.h"

#include "glew/glew.h"
#include "glfw/glfw3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.h"

namespace {
	GLuint lineVao, lineVbo;
}

Renderer::Renderer() {
    initLine();
}

Renderer* Renderer::getInstance() {
    if (!mInstance) {
        mInstance = new Renderer();
    }
    return mInstance;
}

void Renderer::initLine() {
    mShader = Shader("../assets/shaders/default");
    mLineShader = Shader("../assets/shaders/line");

    glGenVertexArrays(1, &lineVao);
    glBindVertexArray(lineVao);
    glGenBuffers(1, &lineVbo);
    glBindBuffer(GL_ARRAY_BUFFER, lineVbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer::drawLine(glm::mat4 transform, glm::mat4 transform2, glm::vec3 color) {
    const float lineVertices[] = {
        transform[3][0], transform[3][1], transform[3][2],
        transform2[3][0], transform2[3][1], transform2[3][2]
    };

    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(lineVao);
    glBindBuffer(GL_ARRAY_BUFFER, lineVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);
    glUseProgram(mLineShader.id);

    const GLint modelLoc = glGetUniformLocation(mLineShader.id, "model");
    const GLint viewLoc = glGetUniformLocation(mLineShader.id, "view");
    const GLint projectionLoc = glGetUniformLocation(mLineShader.id, "projection");
    const GLint colorLoc = glGetUniformLocation(mLineShader.id, "fColor");

    glUniformMatrix4fv(modelLoc, 1, false, &(mCamera.transform[0].x));
    glUniformMatrix4fv(viewLoc, 1, false, &(mCamera.viewMatrix[0].x));
    glUniformMatrix4fv(projectionLoc, 1, false, &(mCamera.projectionMatrix[0].x));
    glUniform3fv(colorLoc, 1, &color.x);

    glDrawArrays(GL_LINES, 0, 3);
}
