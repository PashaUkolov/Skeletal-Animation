#pragma once 
#include <string>

#include "glew/glew.h"
#include "glm/glm.hpp"

#include "tinygltf/tiny_gltf.h"
#include "camera.h"
#include "shader.h"

const int MAX_JOINTS_COUNT = 500;

struct Joint {
    std::vector<Joint*> children;
    Joint* parent;

    glm::mat4 localTransform{ 1.0f };
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

struct Skeleton {
    Joint* root;
    std::vector<Joint*> joints;
};

class AnimatedModel {
public:
    AnimatedModel();
    Skeleton skeleton;
    std::vector<glm::mat4> ibMatices;

    GLuint vao;
    std::map<int, GLuint> vbos;
    GLuint ubo;
    GLuint DATA_UNIFORM_BINDING = 0;

    tinygltf::Model loadModel(const std::string& path);
    tinygltf::Model initModel(const std::string& path);
    std::vector<glm::mat4> readInverseBindMatrix(const tinygltf::Model& model);
    void drawModel(Camera& cam, const tinygltf::Model& model, float timer);
    void fillSkeletonHierarchy(const tinygltf::Model& model, const tinygltf::Node& node, Joint* currentJoint, Joint* parentJoint);
    void readSkeletonHierarchy(const tinygltf::Model& model, Joint* root);

    void draw();
private:
    std::vector<Joint> getAnimationData(const tinygltf::Model& model, float currentTime);
    void drawSkeletonHierarchy(Joint* currentJoint);
    void updateSkeletonHierarchy(Joint* currentJoint, const std::vector<Joint>& pose);
    glm::mat4 computeTRSMatrix(tinygltf::Node node);
    glm::mat4 computeTRSMatrix(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale);

private:
    tinygltf::Model mModel;
    Joint* root = nullptr;
};

