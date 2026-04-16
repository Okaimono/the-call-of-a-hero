#include "pool_allocator.hpp"

void PoolAllocator::init(VulkanContext* context) {
    ctx = context;

    VkMemoryAllocateInfo deviceAllocInfo{};
    deviceAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    deviceAllocInfo.allocationSize = DEVICE_LOCAL_POOL_SIZE;
    deviceAllocInfo.memoryTypeIndex = ctx->findMemoryType(0xFFFFFFFF, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(ctx->device, &deviceAllocInfo, nullptr, &deviceSlab);

    VkMemoryAllocateInfo hostAllocInfo{};
    hostAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    hostAllocInfo.allocationSize = HOST_VISIBLE_POOL_SIZE;
    hostAllocInfo.memoryTypeIndex = ctx->findMemoryType(0xFFFFFFFF, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(ctx->device, &hostAllocInfo, nullptr, &hostSlab);
}

Allocation PoolAllocator::allocateDevice(VkDeviceSize size, VkBufferUsageFlags usage) {
    Allocation allocation{};
    allocation.memory = deviceSlab;
    allocation.offset = deviceOffset;
    allocation.size = size;
    deviceOffset += size;

    return allocation;
}

Allocation PoolAllocator::allocateHost(VkDeviceSize size, VkBufferUsageFlags usage) {
    Allocation allocation{};
    allocation.memory = hostSlab;
    allocation.offset = hostOffset;
    allocation.size = size;
    hostOffset += size;

    return allocation;
}

void PoolAllocator::freeHost(Allocation& allocation) {
    if (allocation.memory != hostSlab) return;
    hostOffset = 0;
}

// void PoolAllocator::pushMemory() {
//     VkBuffer staging = VK_NULL_HANDLE;

//     VkBufferCreateInfo info{};
//     info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//     info.size = size;
//     info.usage = usage;
//     info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//     vkCreateBuffer(ctx.device, &info, nullptr, &staging);

//     void* data;
//     vkMapMemory(ctx.device, stagingBuffer.memory, 0, size, 0, &data);
//     memcpy(data, mesh.vertices.data(), size);
// }