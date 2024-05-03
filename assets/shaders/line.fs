#version 330 core
out vec4 color;

uniform vec3 fColor;

void main() {
    color = vec4(fColor.r, fColor.g, fColor.b, 1.0);
}