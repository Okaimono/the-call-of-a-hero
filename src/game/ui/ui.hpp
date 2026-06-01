#pragma once
#include "vulkan/vulkan_context.hpp"

struct UiVertex {
    glm::vec2 pos;
};

class UI {
public:
    void init(Renderer* renderer) {
        this->renderer = renderer;
        createVertexBuffer();
    }

    Renderer* renderer = nullptr;

    VkBuffer uiBuffer = VK_NULL_HANDLE;
    VkDeviceMemory uiMemory = VK_NULL_HANDLE;

    void createVertexBuffer() {
        UiVertex positions[6] = {
            {{-0.25f, -0.25f}},
            {{ 0.25f, -0.25f}},
            {{ 0.25f,  0.25f}},
            {{-0.25f, -0.25f}},
            {{ 0.25f,  0.25f}},
            {{-0.25f,  0.25f}}
        };

        VkDeviceSize size = sizeof(positions);
        renderer->createBuffer(size,
                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     uiBuffer, uiMemory);
        void* data;
        vkMapMemory(renderer->ctx->device, uiMemory, 0, size, 0, &data);
        memcpy(data, positions, size);
        vkUnmapMemory(renderer->ctx->device, uiMemory);
    }
};