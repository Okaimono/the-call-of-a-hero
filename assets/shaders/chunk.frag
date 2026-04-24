#version 450

layout(set = 0, binding = 2) uniform sampler2D texAtlas;
layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texAtlas, fragUV);
}