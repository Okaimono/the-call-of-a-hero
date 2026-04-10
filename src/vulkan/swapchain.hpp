#pragma once
#include "../vulkan_includes.hpp"
#include "vulkan_context.hpp"

class Swapchain {
public:
    VkSwapchainKHR swapchain      = VK_NULL_HANDLE;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    VkFormat format;
    VkExtent2D extent;

    VkImage depthImage            = VK_NULL_HANDLE;
    VkDeviceMemory depthMemory    = VK_NULL_HANDLE;
    VkImageView depthImageView    = VK_NULL_HANDLE;

    void init(VulkanContext& ctx) {
        createSwapchain(ctx);
        createImageViews(ctx);
        createDepthResources(ctx);
    }

    void cleanup(VulkanContext& ctx) {
        vkDestroyImageView(ctx.device, depthImageView, nullptr);
        vkDestroyImage(ctx.device, depthImage, nullptr);
        vkFreeMemory(ctx.device, depthMemory, nullptr);
        for (auto iv : imageViews)
            vkDestroyImageView(ctx.device, iv, nullptr);
        vkDestroySwapchainKHR(ctx.device, swapchain, nullptr);
    }

private:
    void createSwapchain(VulkanContext& ctx) {
        VkSurfaceCapabilitiesKHR caps;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physicalDevice, ctx.surface, &caps);

        format = VK_FORMAT_B8G8R8A8_SRGB;
        extent = caps.currentExtent;

        VkSwapchainCreateInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        info.surface = ctx.surface;
        info.minImageCount = caps.minImageCount + 1;
        info.imageFormat = format;
        info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        info.imageExtent = extent;
        info.imageArrayLayers = 1;
        info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.preTransform = caps.currentTransform;
        info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        info.clipped = VK_TRUE;

        if (vkCreateSwapchainKHR(ctx.device, &info, nullptr, &swapchain) != VK_SUCCESS)
            throw std::runtime_error("failed to create swapchain");

        uint32_t count;
        vkGetSwapchainImagesKHR(ctx.device, swapchain, &count, nullptr);
        images.resize(count);
        vkGetSwapchainImagesKHR(ctx.device, swapchain, &count, images.data());
        std::cout << "swapchain created" << std::endl;
    }

    void createImageViews(VulkanContext& ctx) {
        imageViews.resize(images.size());
        for (size_t i = 0; i < images.size(); i++) {
            VkImageViewCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            info.image = images[i];
            info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            info.format = format;
            info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            info.subresourceRange.levelCount = 1;
            info.subresourceRange.layerCount = 1;
            vkCreateImageView(ctx.device, &info, nullptr, &imageViews[i]);
        }
        std::cout << "image views created" << std::endl;
    }

    void createDepthResources(VulkanContext& ctx) {
        VkImageCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.extent.width = extent.width;
        info.extent.height = extent.height;
        info.extent.depth = 1;
        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.format = VK_FORMAT_D32_SFLOAT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateImage(ctx.device, &info, nullptr, &depthImage);

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(ctx.device, depthImage, &memReqs);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = ctx.findMemoryType(memReqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(ctx.device, &allocInfo, nullptr, &depthMemory);
        vkBindImageMemory(ctx.device, depthImage, depthMemory, 0);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = depthImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_D32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(ctx.device, &viewInfo, nullptr, &depthImageView);
    }
};