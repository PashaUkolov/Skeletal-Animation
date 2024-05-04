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
#include <set>
#include <list>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "shader.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tinygltf/tiny_gltf.h"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

const int WIDTH = 1000;
const int HEIGHT = 1000;

const int MAX_JOINTS = 1000;
const int MAX_WEIGHTS = 4;
const int DATA_UNIFORM_BINDING = 0;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float zoomLevel = 0.0f;

Shader lineShader{};
Shader shader{};

struct Camera {
    glm::mat4 transform;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
};

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    zoomLevel += yoffset;
}

tinygltf::Model loadModel(const std::string& path) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
    return model;
}

float getFrameTime() {
    double currentTime = glfwGetTime();
    deltaTime = currentTime - lastFrame;
    lastFrame = currentTime;
    return deltaTime;
}

void printMatrix(const glm::mat4& matrix) {
    //system("cls");
    printf("MATRIX ========\n");
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            printf("%.1f ", matrix[i][j]);
        }
        printf("\n");
    }
}

int currentFrame;
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
        currentFrame += 1;
        printf("frame %d\n", currentFrame);
    }

    if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
        currentFrame -= 1;
        if (currentFrame < 0) {
            currentFrame = 0;
        }
        printf("frame %d\n", currentFrame);
    }
}

Camera cam;
void updateCamera(Camera& camera, float timer) {
    camera.transform = glm::mat4(1.0f);
    camera.transform = glm::rotate(camera.transform, glm::radians(timer), glm::vec3(0.0f, 1.0f, 0.0f));
    camera.viewMatrix = glm::mat4(1.0f);
    camera.viewMatrix = glm::lookAt(glm::vec3(0.0f, 5.0f + zoomLevel, -7.0f + -zoomLevel), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0, 1, 0));
    camera.projectionMatrix = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
}

GLuint lineVao, lineVbo;
void initLine() {
    glGenVertexArrays(1, &lineVao);
    glBindVertexArray(lineVao);
    glGenBuffers(1, &lineVbo);
    glBindBuffer(GL_ARRAY_BUFFER, lineVbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void drawLine(glm::mat4 transform, glm::mat4 transform2, glm::vec3 color) {
    glm::vec3 scale;
    glm::quat rotation;
    glm::vec3 translation;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(transform, scale, rotation, translation, skew, perspective);

    glm::vec3 scale2;
    glm::quat rotation2;
    glm::vec3 translation2;
    glm::vec3 skew2;
    glm::vec4 perspective2;
    glm::decompose(transform2, scale2, rotation2, translation2, skew2, perspective2);
    /*float lineVertices[] = {
        0.0f, 0.0f, 0.0f,
        0.0f, 0.1f, 0.0f
    };*/
    float lineVertices[] = {
        translation.x, translation.y, translation.z,
        translation2.x, translation2.y, translation2.z
    };

    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(lineVao);
    glBindBuffer(GL_ARRAY_BUFFER, lineVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);
    glUseProgram(lineShader.id);

    GLuint modelLoc = glGetUniformLocation(lineShader.id, "model");
    GLuint viewLoc = glGetUniformLocation(lineShader.id, "view");
    GLuint projectionLoc = glGetUniformLocation(lineShader.id, "projection");
    GLuint colorLoc = glGetUniformLocation(lineShader.id, "fColor");

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = cam.transform /** transform*/;

    glUniformMatrix4fv(modelLoc, 1, false, &(modelMatrix[0].x));
    glUniformMatrix4fv(viewLoc, 1, false, &(cam.viewMatrix[0].x));
    glUniformMatrix4fv(projectionLoc, 1, false, &(cam.projectionMatrix[0].x));
    glUniform3fv(colorLoc, 1, &color.x);

    glDrawArrays(GL_LINES, 0, 3);
}

struct Joint {
    std::vector<Joint*> children;
    Joint* parent;

    glm::mat4 localTransform{1.0f};
    glm::mat4 globalTransform{ 1.0f };

    glm::mat4 inverseBindTransform;
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
    std::string name;
    int nodeId;

    void addChild(Joint* childNode) {
        children.push_back(childNode);
    }
};

GLuint vao;
std::map<int, GLuint> vbos;
tinygltf::Model initModel(const std::string& path) {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    tinygltf::Model model = loadModel(path);

    for (size_t i = 0; i < model.bufferViews.size(); i++) {
        tinygltf::BufferView bufferView = model.bufferViews[i];
        if (bufferView.target == 0) {
            continue;
        }
        GLuint modelVBO{};
        tinygltf::Buffer buffer = model.buffers[bufferView.buffer];
        glGenBuffers(1, &modelVBO);
        vbos[i] = modelVBO;
        glBindBuffer(bufferView.target, modelVBO);
        glBufferData(bufferView.target, bufferView.byteLength, &buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);
    }

    tinygltf::Scene& scene = model.scenes[model.defaultScene];
    auto& mesh = model.meshes[0];
    for (size_t i = 0; i < mesh.primitives.size(); i++) {
        tinygltf::Primitive primitive = mesh.primitives[i];

        for (auto& attribute : primitive.attributes) {
            tinygltf::Accessor accessor = model.accessors[attribute.second];
            glBindBuffer(GL_ARRAY_BUFFER, vbos[accessor.bufferView]);
            if (attribute.first == "POSITION") {
                int size = accessor.type;
                bool isNormalized = accessor.normalized ? GL_TRUE : GL_FALSE;
                int stride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);

                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, size, GL_FLOAT, isNormalized, stride, (void*)(accessor.byteOffset));
            }
            if (attribute.first == "NORMAL") {
                int size = accessor.type;
                int stride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);

                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, size, GL_FLOAT, accessor.normalized ? GL_TRUE : GL_FALSE, stride, (void*)(accessor.byteOffset));
            }
            if (attribute.first == "JOINTS_0") {
                int size = accessor.type;
                int stride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);

                glEnableVertexAttribArray(3);
                glVertexAttribPointer(3, size, GL_FLOAT, accessor.normalized ? GL_TRUE : GL_FALSE, stride, (void*)(accessor.byteOffset));
            }
            if (attribute.first == "WEIGHTS_0") {
                int size = accessor.type;
                int stride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);

                glEnableVertexAttribArray(4);
                glVertexAttribPointer(4, size, GL_FLOAT, accessor.normalized ? GL_TRUE : GL_FALSE, stride, (void*)(accessor.byteOffset));
            }
        }
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return model;
}

void drawModel(const tinygltf::Model& model, float timer) {
    glBindVertexArray(vao);
    glUseProgram(shader.id);

    auto& mesh = model.meshes[0];

    GLuint modelLoc = glGetUniformLocation(shader.id, "model");
    GLuint viewLoc = glGetUniformLocation(shader.id, "view");
    GLuint projectionLoc = glGetUniformLocation(shader.id, "projection");

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
    modelMatrix = cam.transform * modelMatrix;
    glUniformMatrix4fv(modelLoc, 1, false, &(modelMatrix[0].x));
    glUniformMatrix4fv(viewLoc, 1, false, &(cam.viewMatrix[0].x));
    glUniformMatrix4fv(projectionLoc, 1, false, &(cam.projectionMatrix[0].x));

    for (int i = 0; i < mesh.primitives.size(); ++i) {
        tinygltf::Primitive primitive = mesh.primitives[i];
        tinygltf::Accessor accessor = model.accessors[primitive.indices];
        glBindBuffer(GL_ARRAY_BUFFER, vbos[accessor.bufferView]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[accessor.bufferView]);
        glDrawElements(GL_TRIANGLES, accessor.count, accessor.componentType, BUFFER_OFFSET(accessor.byteOffset));
    }
}

glm::mat4 computeTRSMatrix(tinygltf::Node node) {
    glm::vec3 translation{0.0f};
    glm::quat rotation{};
    glm::vec3 scale{1.0f};

    if (node.translation.size()) {
        translation = { (float)node.translation[0], (float)node.translation[1], (float)node.translation[2] };
    }
    if (node.rotation.size()) {
        rotation = { (float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2], (float)node.rotation[3] };
    }
    if (node.scale.size()) {
        scale = { (float)node.scale[0], (float)node.scale[1], (float)node.scale[2] };
    }
         
    const glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translation);
    const glm::mat4 rotationMatrix = glm::toMat4(rotation);
    const glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

    return translationMatrix * scaleMatrix * rotationMatrix;
}

glm::mat4 computeTRSMatrix(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale) {
    const glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translation);
    const glm::mat4 rotationMatrix = glm::toMat4(rotation);
    const glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

    return translationMatrix * scaleMatrix * rotationMatrix;
}

void fillSkeletonHierarchy(const tinygltf::Model& model, const tinygltf::Node& node, Joint* currentJoint, Joint* parentJoint) {
    currentJoint->localTransform = computeTRSMatrix(node);
	if (parentJoint) {
		currentJoint->globalTransform = parentJoint->globalTransform * currentJoint->localTransform;
    }
    else {
        currentJoint->globalTransform = currentJoint->localTransform;
    }
    currentJoint->name = node.name;

    for (auto& child : node.children) {
        Joint* childJonit = nullptr;
        childJonit = new Joint();
        childJonit->nodeId = child;
        childJonit->parent = currentJoint;
        currentJoint->addChild(childJonit);
        fillSkeletonHierarchy(model, model.nodes[child], childJonit, currentJoint);
    }
}

void readSkeletonHierarchy(const tinygltf::Model& model, Joint* root) {
    int rootJointIndex = -1;
    for (size_t i = 0; i < model.skins.size(); i++) {
        auto& skin = model.skins[i];
        for (size_t j = 0; j < skin.joints.size(); j++) {
            if (model.nodes[skin.joints[j]].name == "root") {
                rootJointIndex = skin.joints[j];
            }
        }
    }
    if (rootJointIndex == -1) {
        return;
    }
    root->nodeId = rootJointIndex;
    fillSkeletonHierarchy(model, model.nodes[rootJointIndex], root, nullptr);
}

std::vector<Joint> getAnimationData(const tinygltf::Model& model, float currentTime) {
	std::vector<Joint> pose(model.nodes.size());
	auto& animation = model.animations[0];
	for (int c = 0; c < animation.channels.size(); c++) {
		auto& channel = animation.channels[c];
		auto& sampler = animation.samplers[channel.sampler];
		auto& inputAccessor = model.accessors[sampler.input];
		auto& outputAccessor = model.accessors[sampler.output];
		auto& inputBufferView = model.bufferViews[inputAccessor.bufferView];
		auto& inputBuffer = model.buffers[inputBufferView.buffer];
		auto& outputBufferView = model.bufferViews[outputAccessor.bufferView];
		auto& outputBuffer = model.buffers[outputBufferView.buffer];

		//printf("node %d: %s\n", channel.target_node, model.nodes[channel.target_node].name.c_str());
		//printf(" channel: %s \n", channel.target_path.c_str());

		std::vector<float> frameTimes(inputAccessor.count);
		memcpy(&frameTimes[0], &inputBuffer.data[inputBufferView.byteOffset], sizeof(float) * frameTimes.size());

        std::vector<glm::vec3> translationValue(outputAccessor.count);
        std::vector<glm::quat> rotationValue(outputAccessor.count);
        std::vector<glm::vec3> scaleValue(outputAccessor.count);

        if (channel.target_path == "translation") {
            memcpy(&translationValue[0], &outputBuffer.data[outputBufferView.byteOffset], sizeof(glm::vec3) * translationValue.size());
            for (size_t i = 0; i < translationValue.size(); i++) {
                //printf("   time: %f | vec x %f, y %f, z %f \n", frameTimes[i], translationValue[i].x, translationValue[i].y, translationValue[i].z);
            }
        }
        else if (channel.target_path == "rotation"){
            memcpy(&rotationValue[0], &outputBuffer.data[outputBufferView.byteOffset], sizeof(glm::quat) * rotationValue.size());
            for (size_t i = 0; i < rotationValue.size(); i++) {
                //printf("   time: %f | vec x %f, y %f, z %f w %f \n", frameTimes[i], rotationValue[i].x, rotationValue[i].y, rotationValue[i].z, rotationValue[i].w);
            }
        }
        else if (channel.target_path == "scale") {
            memcpy(&scaleValue[0], &outputBuffer.data[outputBufferView.byteOffset], sizeof(glm::vec3) * scaleValue.size());
            for (size_t i = 0; i < scaleValue.size(); i++) {
                //printf("   time: %f | vec x %f, y %f, z %f \n", frameTimes[i], scaleValue[i].x, scaleValue[i].y, scaleValue[i].z);
            }
        }

		float previousTime = 0.0f;
		float nextTime = 0.0f;
        for (size_t i = 0; i < frameTimes.size(); i++) {
            if (frameTimes[i] < currentTime) {
                previousTime = frameTimes[i];
            }

            if (frameTimes[i] > currentTime) {
                nextTime = frameTimes[i];
                break;
            }
        }

		for (size_t k = 0; k < frameTimes.size(); k++) {
            if (frameTimes[k] == previousTime) {
                if (channel.target_path == "translation") {
                    pose[channel.target_node].nodeId = channel.target_node;
                    pose[channel.target_node].translation = translationValue[k];
                    pose[channel.target_node].name = model.nodes[channel.target_node].name;
                }
                else if (channel.target_path == "rotation") {
                    pose[channel.target_node].nodeId = channel.target_node;
                    pose[channel.target_node].rotation = rotationValue[k];
                    pose[channel.target_node].name = model.nodes[channel.target_node].name;
                }
                else if (channel.target_path == "scale") {
                    pose[channel.target_node].nodeId = channel.target_node;
                    pose[channel.target_node].scale = scaleValue[k];
                    pose[channel.target_node].name = model.nodes[channel.target_node].name;
                }
            }
		}
	}
    return pose;
}

void drawSkeletonHierarchy(Joint* currentJoint) {
    if (currentJoint->parent) {
        if (currentJoint->parent->name != "root") {
			drawLine(currentJoint->parent->globalTransform, currentJoint->globalTransform, { 1.0f, 1.0f, 1.0f });
        }
    }
    for (auto joint : currentJoint->children) {
        drawSkeletonHierarchy(joint);
    }
}

void updateSkeletonHierarchy(Joint* currentJoint, const std::vector<Joint>& joints) {
	auto t = joints[currentJoint->nodeId].translation;
	auto r = joints[currentJoint->nodeId].rotation;
	auto s = joints[currentJoint->nodeId].scale;

	currentJoint->localTransform = computeTRSMatrix(t, r, s);
	if (currentJoint->parent) {
		currentJoint->globalTransform = currentJoint->parent->globalTransform * currentJoint->localTransform;
    }
    else {
        currentJoint->globalTransform = currentJoint->localTransform;
    }
	for (auto& child : currentJoint->children) {
		updateSkeletonHierarchy(child, joints);
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

    shader = Shader("../assets/shaders/default");
    lineShader = Shader("../assets/shaders/line");
    
    float timer = 0.0f;
    auto& model = initModel("../assets/models/mannequin.gltf");
    initLine();

    Joint* root = nullptr;
    root = new Joint();
    readSkeletonHierarchy(model, root);

    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);

    while (!glfwWindowShouldClose(window)) {
        double frameBegin = glfwGetTime();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        timer += getFrameTime();
        
        updateCamera(cam, timer*30.0f);



        //drawModel(model, timer);

		std::vector<Joint> & joints = getAnimationData(model, fmod(timer, 0.8f));
        updateSkeletonHierarchy(root, joints);
        drawSkeletonHierarchy(root);

        double frameEnd = glfwGetTime();
        char title[256];
        sprintf(title, "frame time: %.1f ms", (frameEnd - frameBegin) * 1000.0f);
        glfwSetWindowTitle(window, title);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}