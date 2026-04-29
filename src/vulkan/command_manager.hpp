#pragma once
#include "vulkan_context.hpp"

class CommandManager {
public:
    VkCommandPool commandPool = VK_NULL_HANDLE;

    void init(VulkanContext* ctx) {
        this->ctx = ctx;

        createCommandPool();
    }

    VkCommandBuffer beginOneShot() {
        VkCommandBufferAllocateInfo info{};
        info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.commandPool        = commandPool;
        info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandBufferCount = 1;
        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(ctx->device, &info, &cmd);
        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &begin);
        return cmd;
    }

    void endOneShot(VkCommandBuffer cmd) {
        vkEndCommandBuffer(cmd);
        VkSubmitInfo submit{};
        submit.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers    = &cmd;
        vkQueueSubmit(ctx->graphicsQueue, 1, &submit, VK_NULL_HANDLE);
        vkQueueWaitIdle(ctx->graphicsQueue);
        vkFreeCommandBuffers(ctx->device, commandPool, 1, &cmd);
    }

    void cleanup() {
        vkDestroyCommandPool(ctx->device, commandPool, nullptr);
    }

private:
    VulkanContext* ctx = nullptr;

    void createCommandPool() {
        VkCommandPoolCreateInfo info{};
        info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.queueFamilyIndex = getGraphicsFamily();
        info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        if (vkCreateCommandPool(ctx->device, &info, nullptr, &commandPool) != VK_SUCCESS)
            throw std::runtime_error("failed to create command pool");
    }

    uint32_t getGraphicsFamily() {
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(ctx->physicalDevice, &count, nullptr);
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(ctx->physicalDevice, &count, families.data());
        for (uint32_t i = 0; i < count; i++)
            if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                return i;
        throw std::runtime_error("no graphics queue family");
    }
};