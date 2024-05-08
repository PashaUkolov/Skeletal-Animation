#include "renderer.h"
#include "shader.h"

#include "glew/glew.h"
#include "glfw/glfw3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.h"

static GLuint lineVao, lineVbo;

Renderer::Renderer() {
    initLine();
}

Renderer::~Renderer() {
}

Renderer* Renderer::getInstance() {
    if (!instance) {
        instance = new Renderer();
    }
    return instance;
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
    float lineVertices[] = {
        transform[3][0], transform[3][1], transform[3][2],
        transform2[3][0], transform2[3][1], transform2[3][2]
    };

    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(lineVao);
    glBindBuffer(GL_ARRAY_BUFFER, lineVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);
    glUseProgram(mLineShader.id);

    GLuint modelLoc = glGetUniformLocation(mLineShader.id, "model");
    GLuint viewLoc = glGetUniformLocation(mLineShader.id, "view");
    GLuint projectionLoc = glGetUniformLocation(mLineShader.id, "projection");
    GLuint colorLoc = glGetUniformLocation(mLineShader.id, "fColor");

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = mCamera.transform /** transform*/;

    glUniformMatrix4fv(modelLoc, 1, false, &(modelMatrix[0].x));
    glUniformMatrix4fv(viewLoc, 1, false, &(mCamera.viewMatrix[0].x));
    glUniformMatrix4fv(projectionLoc, 1, false, &(mCamera.projectionMatrix[0].x));
    glUniform3fv(colorLoc, 1, &color.x);

    glDrawArrays(GL_LINES, 0, 3);
}
