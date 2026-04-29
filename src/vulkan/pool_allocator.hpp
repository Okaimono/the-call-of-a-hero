#pragma once
#include "vulkan_includes.hpp"
#include "vulkan/vulkan_context.hpp"
#include "vulkan/command_manager.hpp"

#include <cstdint>

struct Slot {
    static constexpr VkDeviceSize slotSize = sizeof(uint32_t) * 50000;
    VkDeviceSize slotOffset;
};

class PoolAllocator {
public:
    static constexpr VkDeviceSize poolSize = 256 << 20;

    VkBuffer ssboBuffer = VK_NULL_HANDLE;
    VkDeviceMemory ssboMemory = VK_NULL_HANDLE;

    void init(VulkanContext* ctx, CommandManager* commandManager) {
        this->ctx = ctx;
        this->commandManager = commandManager;
        createSSBO();
    }

    Slot reserveSlot() {
        Slot slot;
        slot.slotOffset = ssboOffset;
        ssboOffset += slot.slotSize;
        return slot;
    }

    void updateSlot(Slot& slot, const std::vector<uint32_t>& slotData) {
        std::vector<uint32_t> newData;
        newData.resize(50000, 0);
        for (int i = 0; i < slotData.size(); i++) {
            newData[i] = slotData[i];
        }

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

        const VkDeviceSize stagingPoolSize = newData.size() * sizeof(uint32_t);
        createBuffer(stagingPoolSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            stagingBuffer, stagingMemory);

        void* data;
        vkMapMemory(ctx->device, stagingMemory, 0, VK_WHOLE_SIZE, 0, &data);
        memcpy(data, newData.data(), newData.size() * sizeof(uint32_t));
        vkUnmapMemory(ctx->device, stagingMemory);
        
        VkCommandBuffer cmd = commandManager->beginOneShot();
        VkBufferCopy copy{};
        copy.srcOffset = 0;
        copy.dstOffset = slot.slotOffset;
        copy.size = newData.size() * sizeof(uint32_t);
        vkCmdCopyBuffer(cmd, stagingBuffer, ssboBuffer, 1, &copy);
        commandManager->endOneShot(cmd);

        vkDestroyBuffer(ctx->device, stagingBuffer, nullptr);
        vkFreeMemory(ctx->device, stagingMemory, nullptr);
    }

private:
    VulkanContext* ctx = nullptr;
    CommandManager* commandManager = nullptr;

    VkDeviceSize ssboOffset = 0;

    void createSSBO() {
        createBuffer(poolSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            ssboBuffer, ssboMemory);
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties,
                      VkBuffer& buffer, VkDeviceMemory& memory) {
        VkBufferCreateInfo info{};
        info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size        = size;
        info.usage       = usage;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(ctx->device, &info, nullptr, &buffer);

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(ctx->device, buffer, &memReqs);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memReqs.size;
        allocInfo.memoryTypeIndex = ctx->findMemoryType(memReqs.memoryTypeBits, properties);
        vkAllocateMemory(ctx->device, &allocInfo, nullptr, &memory);
        vkBindBufferMemory(ctx->device, buffer, memory, 0);
    }
};