#pragma once
#include "vulkan_includes.h"

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
};

class VertexBuffer {
public:
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    void createVertexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, const std::vector<Vertex>& vertices);
    void bindVertexBuffer(VkCommandBuffer cmd);

private:
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexMemory;
    VkDevice device;
    VkPhysicalDevice physicalDevice;
};