#version 450

layout(location = 0) in vec2 inPos;

layout(location = 0) out vec4 fragColor;

void main() {
    gl_Position = vec4(inPos, 0.0, 1.0);
    fragColor = vec4(1.0, 0.0, 0.0, 1.0); // red
}