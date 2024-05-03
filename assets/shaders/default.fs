#version 330 core
out vec4 color;

in vec3 normal;

float map(float value, float min1, float max1, float min2, float max2) {
    return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

void main() {
    color = vec4(clamp(normal.x, 0.1, 1.0), clamp(normal.y, 0.1, 1.0), clamp(normal.z, 0.1, 1.0), 1.0);
}