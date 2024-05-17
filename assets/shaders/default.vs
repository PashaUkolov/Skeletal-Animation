#version 330 core
layout (location = 0) in vec3 iPos;
layout (location = 1) in vec3 iNormal;
layout (location = 2) in vec2 iTexCoord;
layout (location = 3) in ivec4 jointIDs;
layout (location = 4) in vec4 weights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 normal;

layout (std140) uniform data {
    mat4 skinningMatrices[1000];
};

void main() {
    vec3 totalPos = vec3(0.0);
    vec3 totalNormal = vec3(0.0);
    for(int i = 0; i < 4; i += 1) {
        mat4 jointTransform = skinningMatrices[int(jointIDs[i])];
        vec3 posePosition = (jointTransform * vec4(iPos, 1.0)).xyz;
        vec3 poseNormal = (jointTransform * vec4(iNormal, 0.0)).xyz;
        totalPos += posePosition * weights[i];
        totalNormal += poseNormal * weights[i];
    }
    gl_Position = projection * view * model * vec4(totalPos, 1.0);
    normal = totalNormal;

    // gl_Position = projection * view * model * vec4(iPos, 1.0);
    // normal = iNormal;
}