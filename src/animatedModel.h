#pragma once 
#include <string>

#include "glew/glew.h"
#include "glm/glm.hpp"

#include "tinygltf/tiny_gltf.h"
#include "camera.h"

constexpr int MAX_JOINTS_COUNT = 500;

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
    std::vector<glm::mat4> ibMatrices;

    void updateMeshAtrributes(const tinygltf::Mesh& mesh, const tinygltf::Model& model);
    static tinygltf::Model loadModel(const std::string& path);
    tinygltf::Model initModel(const std::string& path);
    static std::vector<glm::mat4> readInverseBindMatrix(const tinygltf::Model& model);
    void fillSkeletonHierarchy(const tinygltf::Model& model, const tinygltf::Node& node, Joint* currentJoint, Joint* parentJoint);
    void readSkeletonHierarchy(const tinygltf::Model& model, Joint* root);

    void drawModel(Camera& cam, const tinygltf::Model& model);
    void draw(const double timer);
private:
    static std::vector<Joint> getAnimationData(const tinygltf::Model& model, double currentTime);
    static void drawSkeletonHierarchy(const Joint* currentJoint);
    static void updateSkeletonHierarchy(Joint* currentJoint, const std::vector<Joint>& pose);
    static glm::mat4 computeTrsMatrix(const tinygltf::Node& node);
    static glm::mat4 computeTrsMatrix(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale);
private:
    GLuint mVao;
    GLuint mEbo;
    std::map<size_t, GLuint> mVbos;
    std::vector<GLuint> mEbos;
    GLuint mUbo;
    GLuint DATA_UNIFORM_BINDING = 0;
    tinygltf::Model mModel;
    Joint* mRoot = nullptr;
};

