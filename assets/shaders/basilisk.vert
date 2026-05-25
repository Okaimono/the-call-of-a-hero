#version 450

layout(location = 0) in vec3 pos;

layout(set = 0, binding = 0) uniform UBO {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec3 color;
} pc;

layout(location = 0) out vec3 fragColor;

void main() {
    fragColor = pc.color;
    gl_Position = ubo.proj * ubo.view * pc.model * vec4(pos, 1.0);
}