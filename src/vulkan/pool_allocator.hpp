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
    void freeHost(Allocation& allocation);

    VkDeviceSize getDeviceOffset() {
        return deviceOffset;
    }

private:
    struct FreeBlock {
        VkDeviceSize offset;
        VkDeviceSize size;
    };
    std::vector<FreeBlock> hostFreeList;

    std::vector<Allocation> allocations;

    VulkanContext* ctx = nullptr;

    VkDeviceMemory deviceSlab;
    VkDeviceMemory hostSlab;

    VkDeviceSize deviceOffset = 0;
    VkDeviceSize hostOffset = 0;

};

/*
Allocate an allocation on the vector.

What we need:
 - A allocation way, to add data, 
 - A way to add up the position in the specific memory
*/