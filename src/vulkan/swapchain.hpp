#pragma once
#include "vulkan_context.hpp"

class Swapchain {
public:
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    VkFormat swapchainFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImageView> imageViews;

    void init(VulkanContext& ctx) {
        createSwapchain(ctx);
        createImageViews(ctx);
    }

    void createSwapchain(VulkanContext& ctx) {
        VkSurfaceCapabilitiesKHR caps;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physicalDevice, ctx.surface, &caps);

        swapchainFormat = VK_FORMAT_B8G8R8A8_SRGB;
        swapchainExtent = caps.currentExtent;

        VkSwapchainCreateInfoKHR info{};
        info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        info.surface          = ctx.surface;
        info.minImageCount    = caps.minImageCount + 1;
        info.imageFormat      = swapchainFormat;
        info.imageColorSpace  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        info.imageExtent      = swapchainExtent;
        info.imageArrayLayers = 1;
        info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.preTransform     = caps.currentTransform;
        info.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        info.presentMode      = VK_PRESENT_MODE_FIFO_KHR;
        info.clipped          = VK_TRUE;

        if (vkCreateSwapchainKHR(ctx.device, &info, nullptr, &swapchain) != VK_SUCCESS)
            throw std::runtime_error("failed to create swapchain");

        uint32_t count;
        vkGetSwapchainImagesKHR(ctx.device, swapchain, &count, nullptr);
        swapchainImages.resize(count);
        vkGetSwapchainImagesKHR(ctx.device, swapchain, &count, swapchainImages.data());
        std::cout << "swapchain created\n";
    }

    void createImageViews(VulkanContext& ctx) {
        imageViews.resize(swapchainImages.size());
        for (size_t i = 0; i < swapchainImages.size(); i++) {
            VkImageViewCreateInfo info{};
            info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            info.image    = swapchainImages[i];
            info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            info.format   = swapchainFormat;
            info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            info.subresourceRange.levelCount = 1;
            info.subresourceRange.layerCount = 1;
            vkCreateImageView(ctx.device, &info, nullptr, &imageViews[i]);
        }
    }

    void cleanup(VulkanContext& ctx) {
        for (auto iv : imageViews) vkDestroyImageView(ctx.device, iv, nullptr);
        vkDestroySwapchainKHR(ctx.device, swapchain, nullptr);
    }
};