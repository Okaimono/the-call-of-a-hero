#pragma once
#include "vulkan_includes.hpp"
#include "vulkan_context.hpp"

class Buffer {
public:
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;

    void createBuffer(VulkanContext& ctx, VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties);
    void deleteBuffer(VulkanContext& ctx);
};