#include "animatedModel.h"

#include "renderer.h"
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "animatedModel.h"
#include "renderer.h"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

AnimatedModel::AnimatedModel() {
    mModel = initModel("../assets/models/mannequin.gltf");
    root = new Joint();
    readSkeletonHierarchy(mModel, root);
}

tinygltf::Model AnimatedModel::loadModel(const std::string& path) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
    return model;
}

std::vector<glm::mat4> AnimatedModel::readInverseBindMatrix(const tinygltf::Model& model) {
    for (size_t i = 0; i < model.skins.size(); i++) {
        auto& skin = model.skins[i];
        auto& ibMatrixAccessor = model.accessors[skin.inverseBindMatrices];
        std::vector<glm::mat4> ibMatrices(ibMatrixAccessor.count);
        auto& bufferView = model.bufferViews[ibMatrixAccessor.bufferView];
        auto& buffer = model.buffers[bufferView.buffer];
        memcpy(&ibMatrices[0], &buffer.data[bufferView.byteOffset], sizeof(glm::mat4) * ibMatrices.size());
        return ibMatrices;
    }
}

tinygltf::Model AnimatedModel::initModel(const std::string& path) {
    auto big_data_uniform_index = glGetUniformBlockIndex(Renderer::getInstance()->getShader().id, "data");
    glUniformBlockBinding(Renderer::getInstance()->getShader().id, big_data_uniform_index, DATA_UNIFORM_BINDING);

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
        glBufferData(bufferView.target, bufferView.byteLength, &buffer.data.at(0) + bufferView.byteOffset, GL_DYNAMIC_DRAW);
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
                int stride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, size, accessor.componentType, accessor.normalized ? GL_TRUE : GL_FALSE, stride, (void*)(accessor.byteOffset));
            }
            if (attribute.first == "NORMAL") {
                int size = accessor.type;
                int stride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, size, accessor.componentType, accessor.normalized ? GL_TRUE : GL_FALSE, stride, (void*)(accessor.byteOffset));
            }
            if (attribute.first == "TEXCOORD_0") {
                int size = accessor.type;
                int stride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(2, size, accessor.componentType, accessor.normalized ? GL_TRUE : GL_FALSE, stride, (void*)(accessor.byteOffset));
            }
            if (attribute.first == "JOINTS_0") {
                int size = accessor.type;
                int stride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);
                glEnableVertexAttribArray(3);
                glVertexAttribIPointer(3, size, accessor.componentType, stride, (void*)(accessor.byteOffset));
            }
            if (attribute.first == "WEIGHTS_0") {
                int size = accessor.type;
                int stride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);
                glEnableVertexAttribArray(4);
                glVertexAttribPointer(4, size, accessor.componentType, accessor.normalized ? GL_TRUE : GL_FALSE, stride, (void*)(accessor.byteOffset));
            }
        }
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * MAX_JOINTS_COUNT, NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, DATA_UNIFORM_BINDING, ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    return model;
}

void AnimatedModel::drawModel(Camera& cam, const tinygltf::Model& model, float timer) {
    glm::mat4 jointMatrices[MAX_JOINTS_COUNT] = {};
    for (int i = 0; i < skeleton.joints.size(); i++) {
        ibMatices = readInverseBindMatrix(model);
        skeleton.joints[i]->inverseBindTransform = ibMatices[i];
        jointMatrices[i] = skeleton.joints[i]->globalTransform * skeleton.joints[i]->inverseBindTransform;
    }

    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4) * MAX_JOINTS_COUNT, glm::value_ptr(jointMatrices[0]));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindVertexArray(vao);
    glUseProgram(Renderer::getInstance()->getShader().id);

    auto& mesh = model.meshes[0];

    GLuint modelLoc = glGetUniformLocation(Renderer::getInstance()->getShader().id, "model");
    GLuint viewLoc = glGetUniformLocation(Renderer::getInstance()->getShader().id, "view");
    GLuint projectionLoc = glGetUniformLocation(Renderer::getInstance()->getShader().id, "projection");

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

glm::mat4 AnimatedModel::computeTRSMatrix(tinygltf::Node node) {
    glm::vec3 translation{ 0.0f };
    glm::quat rotation{};
    glm::vec3 scale{ 1.0f };

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

glm::mat4 AnimatedModel::computeTRSMatrix(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale) {
    const glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translation);
    const glm::mat4 rotationMatrix = glm::toMat4(rotation);
    const glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

    return translationMatrix * scaleMatrix * rotationMatrix;
}

void AnimatedModel::fillSkeletonHierarchy(const tinygltf::Model& model, const tinygltf::Node& node, Joint* currentJoint, Joint* parentJoint) {
    currentJoint->localTransform = computeTRSMatrix(node);
    if (parentJoint) {
        currentJoint->globalTransform = parentJoint->globalTransform * currentJoint->localTransform;
    }
    else {
        currentJoint->globalTransform = currentJoint->localTransform;
    }
    currentJoint->name = node.name;
    skeleton.joints.push_back(currentJoint);

    for (auto& child : node.children) {
        Joint* childJonit = nullptr;
        childJonit = new Joint();
        childJonit->nodeId = child;
        childJonit->parent = currentJoint;
        currentJoint->addChild(childJonit);
        fillSkeletonHierarchy(model, model.nodes[child], childJonit, currentJoint);
    }
}

void AnimatedModel::readSkeletonHierarchy(const tinygltf::Model& model, Joint* root) {
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
    skeleton.root = root;
    fillSkeletonHierarchy(model, model.nodes[rootJointIndex], root, nullptr);
}

std::vector<Joint> AnimatedModel::getAnimationData(const tinygltf::Model& model, float currentTime) {
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
        }
        else if (channel.target_path == "rotation") {
            memcpy(&rotationValue[0], &outputBuffer.data[outputBufferView.byteOffset], sizeof(glm::quat) * rotationValue.size());
        }
        else if (channel.target_path == "scale") {
            memcpy(&scaleValue[0], &outputBuffer.data[outputBufferView.byteOffset], sizeof(glm::vec3) * scaleValue.size());
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

void AnimatedModel::drawSkeletonHierarchy(Joint* currentJoint) {
    if (currentJoint->parent) {
        if (currentJoint->parent->name != "root") {
            Renderer::getInstance()->drawLine(currentJoint->parent->globalTransform, currentJoint->globalTransform, { 1.0f, 1.0f, 1.0f });
        }
    }
    for (auto joint : currentJoint->children) {
        drawSkeletonHierarchy(joint);
    }
}

void AnimatedModel::updateSkeletonHierarchy(Joint* currentJoint, const std::vector<Joint>& pose) {
    auto t = pose[currentJoint->nodeId].translation;
    auto r = pose[currentJoint->nodeId].rotation;
    auto s = pose[currentJoint->nodeId].scale;

    currentJoint->localTransform = computeTRSMatrix(t, r, s);
    if (currentJoint->parent) {
        currentJoint->globalTransform = currentJoint->parent->globalTransform * currentJoint->localTransform;
    }
    else {
        currentJoint->globalTransform = currentJoint->localTransform;
    }
    for (auto& child : currentJoint->children) {
        updateSkeletonHierarchy(child, pose);
    }
}

void AnimatedModel::draw() {
    std::vector<Joint>& pose = getAnimationData(mModel, 0.0f/*fmod(timer, 0.8f)*/);

    drawModel(Renderer::getInstance()->getCamera(), mModel, 0.0f);

    updateSkeletonHierarchy(root, pose);
    drawSkeletonHierarchy(root);
}