#version 330 core
layout (location = 0) in vec3 iPos;
layout (location = 1) in vec3 iNormal;
layout (location = 2) in vec2 iTexCoord;
layout (location = 3) in ivec4 jointIDs;
layout (location = 4) in vec4 weights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
//uniform mat4 skinningMatrices[300];

out vec3 normal;

layout (std140) uniform data {
    mat4 skinningMatrices[1000];
};

void main() {
    // vec3 totalPos = vec3(0.0);
    // for(int i = 0; i < 4; i += 1) {
    //     mat4 jointTransform = skinningMatrices[int(jointIDs[i])];
    //     vec3 posePosition = (jointTransform * vec4(iPos, 1.0)).xyz;
    //     totalPos += posePosition * weights[i];
    // }
    // gl_Position = projection * view * model * vec4(totalPos, 1.0);

    
    vec3 model_position = vec3(0);
    for (int i = 0; i < 3; i += 1) {
		int joint_id = int(jointIDs[i]);
		float weight = weights[i];
		mat4 skinning_matrix = skinningMatrices[joint_id];
		vec3 pose_position = (skinning_matrix * vec4 (iPos, 1)).xyz;
		model_position += pose_position * weight;
	}
    gl_Position = projection * view * model * vec4(model_position.xyz, 1.0);

    // mat4 skinMat =
    //     weights.x * skinningMatrices[int(jointIDs.x)] +
    //     weights.y * skinningMatrices[int(jointIDs.y)] +
    //     weights.z * skinningMatrices[int(jointIDs.z)] +
    //     weights.w * skinningMatrices[int(jointIDs.w)];
    // vec4 worldPosition = skinMat * vec4(iPos, 1.0);
    // vec4 cameraPosition = view * model * worldPosition;
    // gl_Position = projection * cameraPosition;

    normal = iNormal;
}