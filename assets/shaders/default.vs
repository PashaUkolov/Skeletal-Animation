#version 330 core
layout (location = 0) in vec3 iPos;
layout (location = 1) in vec3 iNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 normal;

layout (std140) uniform data {
    mat4 skinning_matrices[10];
};

void main() {
    gl_Position = projection * view * model * vec4(iPos.x, iPos.y, iPos.z, 1.0);
    normal = iNormal;
}