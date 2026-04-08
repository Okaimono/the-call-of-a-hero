#include "VertexBuffer.h"

uint32_t findMemoryType(uint32_t typeFilter, VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    throw std::runtime_error("failed to find suitable memory type");
}

void VertexBuffer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(device, &info, nullptr, &vertexBuffer);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device, vertexBuffer, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, physicalDevice, properties);

    vkAllocateMemory(device, &allocInfo, nullptr, &vertexMemory);
    vkBindBufferMemory(device, vertexBuffer, vertexMemory, 0);
}

void VertexBuffer::createVertexBuffer(VkDevice dev, VkPhysicalDevice physicalDev, const std::vector<Vertex>& vertices) {
    physicalDevice = physicalDev;
    device = dev;
    VkDeviceSize size = sizeof(vertices[0]) * vertices.size();
    createBuffer(size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    void* data;
    vkMapMemory(device, vertexMemory, 0, size, 0, &data);
    memcpy(data, vertices.data(), size);
    vkUnmapMemory(device, vertexMemory);
}

void VertexBuffer::bindVertexBuffer(VkCommandBuffer cmd) {
    VkDeviceSize offsets[] = {0};
    VkBuffer vertexBuffers[] = {vertexBuffer};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
}