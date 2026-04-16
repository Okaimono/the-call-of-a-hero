#pragma once
#include "vulkan_includes.hpp"

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 uv;
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct ChunkMesh {
    std::vector<Vertex> vertices;
};