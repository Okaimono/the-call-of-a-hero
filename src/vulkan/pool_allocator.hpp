#pragma once
#include "vulkan_includes.hpp"
#include "vulkan_context.hpp"

constexpr VkDeviceSize DEVICE_LOCAL_POOL_SIZE = 256 * 1024 * 1024; // 256MB
constexpr VkDeviceSize HOST_VISIBLE_POOL_SIZE  = 64  * 1024 * 1024; // 64MB

struct Allocation {
    VkDeviceMemory memory;  // parent slab
    VkDeviceSize   offset = 0;  // where you live in it
    VkDeviceSize   size;
};
class PoolAllocator {
public:

    void init(VulkanContext* context);
    Allocation allocateDevice(VkDeviceSize size, VkBufferUsageFlags usage);
    Allocation allocateHost(VkDeviceSize size, VkBufferUsageFlags usage);

private:
    VulkanContext* ctx = nullptr;

    VkDeviceMemory deviceSlab;
    VkDeviceMemory hostSlab;

    VkDeviceSize deviceOffset = 0;
    VkDeviceSize hostOffset = 0;

};