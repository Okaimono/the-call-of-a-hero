#pragma once
#include "vulkan_includes.hpp"
#include "game/basilisk/basilisk.hpp"
#include "vulkan/renderer.hpp"

class Tower {
public:
    float HP = 100.0f;
    glm::vec3 position = glm::vec3(0.0f, 10.0f, 0.0f);

    Segment towerSegment;
    Renderer* renderer;

    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;

    void init(Renderer* renderer) {
        this->renderer = renderer;
        initMesh();
        createVertexBuffer();

    }

    void initMesh() {
        towerSegment.pos = position;
        towerSegment.push.model = glm::translate(glm::mat4(1.0f), position);
        towerSegment.push.model *= glm::scale(glm::mat4(1.0f), glm::vec3(7.0f, 7.0f, 7.0f));
    }

    void createVertexBuffer() {
        BasiliskVertex positions[] = {
            // Front face
            {{-0.5f, -0.5f,  0.5f}}, {{ 0.5f, -0.5f,  0.5f}}, {{ 0.5f,  0.5f,  0.5f}},
            {{-0.5f, -0.5f,  0.5f}}, {{ 0.5f,  0.5f,  0.5f}}, {{-0.5f,  0.5f,  0.5f}},
            // Back face
            {{ 0.5f, -0.5f, -0.5f}}, {{-0.5f, -0.5f, -0.5f}}, {{-0.5f,  0.5f, -0.5f}},
            {{ 0.5f, -0.5f, -0.5f}}, {{-0.5f,  0.5f, -0.5f}}, {{ 0.5f,  0.5f, -0.5f}},
            // Left face
            {{-0.5f, -0.5f, -0.5f}}, {{-0.5f, -0.5f,  0.5f}}, {{-0.5f,  0.5f,  0.5f}},
            {{-0.5f, -0.5f, -0.5f}}, {{-0.5f,  0.5f,  0.5f}}, {{-0.5f,  0.5f, -0.5f}},
            // Right face
            {{ 0.5f, -0.5f,  0.5f}}, {{ 0.5f, -0.5f, -0.5f}}, {{ 0.5f,  0.5f, -0.5f}},
            {{ 0.5f, -0.5f,  0.5f}}, {{ 0.5f,  0.5f, -0.5f}}, {{ 0.5f,  0.5f,  0.5f}},
            // Top face
            {{-0.5f,  0.5f,  0.5f}}, {{ 0.5f,  0.5f,  0.5f}}, {{ 0.5f,  0.5f, -0.5f}},
            {{-0.5f,  0.5f,  0.5f}}, {{ 0.5f,  0.5f, -0.5f}}, {{-0.5f,  0.5f, -0.5f}},
            // Bottom face
            {{-0.5f, -0.5f, -0.5f}}, {{ 0.5f, -0.5f, -0.5f}}, {{ 0.5f, -0.5f,  0.5f}},
            {{-0.5f, -0.5f, -0.5f}}, {{ 0.5f, -0.5f,  0.5f}}, {{-0.5f, -0.5f,  0.5f}},
        };

        VkDeviceSize size = sizeof(positions);
        renderer->createBuffer(size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            buffer, memory);

        void* data;
        vkMapMemory(renderer->ctx->device, memory, 0, size, 0, &data);
        memcpy(data, positions, size);
        vkUnmapMemory(renderer->ctx->device, memory);
    }

};