#include "buffer.hpp"

void Buffer::createBuffer(VulkanContext& ctx, VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties) {
    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(ctx.device, &info, nullptr, &buffer);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(ctx.device, buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = ctx.findMemoryType(memReqs.memoryTypeBits, properties);
    vkAllocateMemory(ctx.device, &allocInfo, nullptr, &memory);
    vkBindBufferMemory(ctx.device, buffer, memory, 0);
}

void Buffer::deleteBuffer(VulkanContext& ctx) {
    if (buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(ctx.device, buffer, nullptr);
    }
    if (memory != VK_NULL_HANDLE) {
        vkFreeMemory(ctx.device, memory, nullptr);
    }
}