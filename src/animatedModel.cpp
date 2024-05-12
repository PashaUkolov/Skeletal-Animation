#include "animatedModel.h"

#include "renderer.h"
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

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
    std::vector<glm::mat4> matrices;
    for (size_t i = 0; i < model.skins.size(); i++) {
        auto& skin = model.skins[i];
        auto& ibMatrixAccessor = model.accessors[skin.inverseBindMatrices];
        matrices.resize(ibMatrixAccessor.count);
        auto& bufferView = model.bufferViews[ibMatrixAccessor.bufferView];
        auto& buffer = model.buffers[bufferView.buffer];
        memcpy(matrices.data(), &buffer.data[bufferView.byteOffset], sizeof(glm::mat4) * matrices.size());
        break;
    }
	return matrices;
}

tinygltf::Model AnimatedModel::initModel(const std::string& path) {
    auto big_data_uniform_index = glGetUniformBlockIndex(Renderer::getInstance()->getShader().id, "data");
    glUniformBlockBinding(Renderer::getInstance()->getShader().id, big_data_uniform_index, DATA_UNIFORM_BINDING);

    glGenVertexArrays(1, &mVao);
    glBindVertexArray(mVao);

    tinygltf::Model model = loadModel(path);

    for (size_t i = 0; i < model.bufferViews.size(); i++) {
        tinygltf::BufferView bufferView = model.bufferViews[i];
        if (bufferView.target == 0) {
            continue;
        }
        GLuint modelVbo{};
        tinygltf::Buffer buffer = model.buffers[bufferView.buffer];
        glGenBuffers(1, &modelVbo);
        mVbos[i] = modelVbo;
        glBindBuffer(bufferView.target, modelVbo);
        glBufferData(bufferView.target, bufferView.byteLength, &buffer.data.at(0) + bufferView.byteOffset, GL_DYNAMIC_DRAW);
    }

    tinygltf::Scene& scene = model.scenes[model.defaultScene];
    auto& mesh = model.meshes[0];
    for (size_t i = 0; i < mesh.primitives.size(); i++) {
        tinygltf::Primitive primitive = mesh.primitives[i];
        for (auto& [attribute, accessorIndex ]: primitive.attributes) {
            tinygltf::Accessor accessor = model.accessors[accessorIndex];
            glBindBuffer(GL_ARRAY_BUFFER, mVbos[accessor.bufferView]);
            if (attribute == "POSITION") {
                int size = accessor.type;
                int stride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, size, accessor.componentType, accessor.normalized ? GL_TRUE : GL_FALSE, stride, (void*)(accessor.byteOffset));
            }
            if (attribute == "NORMAL") {
                int size = accessor.type;
                int stride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, size, accessor.componentType, accessor.normalized ? GL_TRUE : GL_FALSE, stride, (void*)(accessor.byteOffset));
            }
            if (attribute == "TEXCOORD_0") {
                int size = accessor.type;
                int stride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(2, size, accessor.componentType, accessor.normalized ? GL_TRUE : GL_FALSE, stride, (void*)(accessor.byteOffset));
            }
            if (attribute == "JOINTS_0") {
                int size = accessor.type;
                int stride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);
                glEnableVertexAttribArray(3);
                glVertexAttribIPointer(3, size, accessor.componentType, stride, (void*)(accessor.byteOffset));
            }
            if (attribute == "WEIGHTS_0") {
                int size = accessor.type;
                int stride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);
                glEnableVertexAttribArray(4);
                glVertexAttribPointer(4, size, accessor.componentType, accessor.normalized ? GL_TRUE : GL_FALSE, stride, (void*)(accessor.byteOffset));
            }
        }
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &mUbo);
    glBindBuffer(GL_UNIFORM_BUFFER, mUbo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * MAX_JOINTS_COUNT, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, DATA_UNIFORM_BINDING, mUbo);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    return model;
}

void AnimatedModel::drawModel(Camera& cam, const tinygltf::Model& model, float timer) const {
    static glm::mat4 jointMatrices[MAX_JOINTS_COUNT] = {};
    for (int i = 0; i < skeleton.joints.size(); i++) {
        if(i < ibMatrices.size()) {
			skeleton.joints[i]->inverseBindTransform = ibMatrices[i];
			jointMatrices[i] = skeleton.joints[i]->globalTransform * skeleton.joints[i]->inverseBindTransform;
        }
    }

    glBindBuffer(GL_UNIFORM_BUFFER, mUbo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4) * MAX_JOINTS_COUNT, glm::value_ptr(jointMatrices[0]));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindVertexArray(mVao);
    glUseProgram(Renderer::getInstance()->getShader().id);

    auto& mesh = model.meshes[0];

    GLint modelLoc = glGetUniformLocation(Renderer::getInstance()->getShader().id, "model");
    GLint viewLoc = glGetUniformLocation(Renderer::getInstance()->getShader().id, "view");
    GLint projectionLoc = glGetUniformLocation(Renderer::getInstance()->getShader().id, "projection");

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
    modelMatrix = cam.transform * modelMatrix;
    glUniformMatrix4fv(modelLoc, 1, false, &(modelMatrix[0].x));
    glUniformMatrix4fv(viewLoc, 1, false, &(cam.viewMatrix[0].x));
    glUniformMatrix4fv(projectionLoc, 1, false, &(cam.projectionMatrix[0].x));

    for (size_t i = 0; i < mesh.primitives.size(); ++i) {
        tinygltf::Primitive primitive = mesh.primitives[i];
        tinygltf::Accessor accessor = model.accessors[primitive.indices];
        glDrawElements(GL_TRIANGLES, accessor.count, accessor.componentType, BUFFER_OFFSET(accessor.byteOffset));
    }
}

glm::mat4 AnimatedModel::computeTrsMatrix(const tinygltf::Node& node) {
    glm::vec3 translation{ 0.0f };
    glm::quat rotation{};
    glm::vec3 scale{ 1.0f };

    if (!node.translation.empty()) {
        translation = { (float)node.translation[0], (float)node.translation[1], (float)node.translation[2] };
    }
    if (!node.rotation.empty()) {
        rotation = { (float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2], (float)node.rotation[3] };
    }
    if (!node.scale.empty()) {
        scale = { (float)node.scale[0], (float)node.scale[1], (float)node.scale[2] };
    }

    const glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translation);
    const glm::mat4 rotationMatrix = glm::toMat4(rotation);
    const glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

    return translationMatrix * scaleMatrix * rotationMatrix;
}

glm::mat4 AnimatedModel::computeTrsMatrix(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale) {
    const glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translation);
    const glm::mat4 rotationMatrix = glm::toMat4(rotation);
    const glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

    return translationMatrix * scaleMatrix * rotationMatrix;
}

void AnimatedModel::fillSkeletonHierarchy(const tinygltf::Model& model, const tinygltf::Node& node, Joint* currentJoint, Joint* parentJoint) {
    currentJoint->localTransform = computeTrsMatrix(node);
    if (parentJoint) {
        currentJoint->globalTransform = parentJoint->globalTransform * currentJoint->localTransform;
    }
    else {
        currentJoint->globalTransform = currentJoint->localTransform;
    }
    currentJoint->name = node.name;
    skeleton.joints.push_back(currentJoint);

    for (auto& child : node.children) {
        Joint* childJoint = nullptr;
        childJoint = new Joint();
        childJoint->nodeId = child;
        childJoint->parent = currentJoint;
        currentJoint->addChild(childJoint);
        fillSkeletonHierarchy(model, model.nodes[child], childJoint, currentJoint);
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

std::vector<Joint> AnimatedModel::getAnimationData(const tinygltf::Model& model, double currentTime) {
    std::vector<Joint> pose(model.nodes.size());
    auto& animation = model.animations[0];
    for (size_t c = 0; c < animation.channels.size(); c++) {
        auto& channel = animation.channels[c];
        auto& sampler = animation.samplers[channel.sampler];
        auto& inputAccessor = model.accessors[sampler.input];
        auto& outputAccessor = model.accessors[sampler.output];
        auto& inputBufferView = model.bufferViews[inputAccessor.bufferView];
        auto& inputBuffer = model.buffers[inputBufferView.buffer];
        auto& outputBufferView = model.bufferViews[outputAccessor.bufferView];
        auto& outputBuffer = model.buffers[outputBufferView.buffer];

        std::vector<float> frameTimes(inputAccessor.count);
        memcpy(frameTimes.data(), &inputBuffer.data[inputBufferView.byteOffset], sizeof(float) * frameTimes.size());

        std::vector<glm::vec3> translationValue(outputAccessor.count);
        std::vector<glm::quat> rotationValue(outputAccessor.count);
        std::vector<glm::vec3> scaleValue(outputAccessor.count);

        if (channel.target_path == "translation") {
            memcpy(translationValue.data(), &outputBuffer.data[outputBufferView.byteOffset], sizeof(glm::vec3) * translationValue.size());
        }
        else if (channel.target_path == "rotation") {
            memcpy(rotationValue.data(), &outputBuffer.data[outputBufferView.byteOffset], sizeof(glm::quat) * rotationValue.size());
        }
        else if (channel.target_path == "scale") {
            memcpy(scaleValue.data(), &outputBuffer.data[outputBufferView.byteOffset], sizeof(glm::vec3) * scaleValue.size());
        }

        float previousTime = 0.0f;
        for (size_t i = 0; i < frameTimes.size(); i++) {
            if (frameTimes[i] < currentTime) {
                previousTime = frameTimes[i];
            }

            if (frameTimes[i] > currentTime) {
				float nextTime = 0.0f;
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

void AnimatedModel::drawSkeletonHierarchy(const Joint* currentJoint) {
    if (currentJoint->parent) {
        if (currentJoint->parent->name != "root") {
            Renderer::getInstance()->drawLine(currentJoint->parent->globalTransform, currentJoint->globalTransform, { 1.0f, 1.0f, 1.0f });
        }
    }
    for (const auto& joint : currentJoint->children) {
        drawSkeletonHierarchy(joint);
    }
}

void AnimatedModel::updateSkeletonHierarchy(Joint* currentJoint, const std::vector<Joint>& pose) {
    auto t = pose[currentJoint->nodeId].translation;
    auto r = pose[currentJoint->nodeId].rotation;
    auto s = pose[currentJoint->nodeId].scale;

    currentJoint->localTransform = computeTrsMatrix(t, r, s);
    if (currentJoint->parent) {
        currentJoint->globalTransform = currentJoint->parent->globalTransform * currentJoint->localTransform;
    }
    else {
        currentJoint->globalTransform = currentJoint->localTransform;
    }
    for (const auto& child : currentJoint->children) {
        updateSkeletonHierarchy(child, pose);
    }
}

void AnimatedModel::draw(const double timer) {
    const std::vector<Joint>& pose = getAnimationData(mModel, fmod(timer, 0.8));

    drawModel(Renderer::getInstance()->getCamera(), mModel, 0.0f);

    updateSkeletonHierarchy(mRoot, pose);
    drawSkeletonHierarchy(mRoot);
}