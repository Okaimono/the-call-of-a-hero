#pragma once
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "vulkan_context.hpp"
#include "swapchain.hpp"
#include "core/types.hpp"
#include "core/config.hpp"
#include "vulkan/command_manager.hpp"
#include "vulkan/pool_allocator.hpp"
#include "game/world.hpp"
#include "vulkan/render_queue.hpp"
#include "vulkan/renderer_resource_manager.hpp"

#include <functional>
#include <unordered_map>
#include <glm/gtc/matrix_transform.hpp>

struct WorldData {
    Slot slot;
    ChunkCoord coords;
};

class Renderer {
public:
    PoolAllocator allocator;

    VkRenderPass               renderPass     = VK_NULL_HANDLE;
    
    uint32_t chunkPipelineId = 0;
    uint32_t uiPipelineId    = 0;

    VkBuffer uiBuffer = VK_NULL_HANDLE;
    VkDeviceMemory uiMemory = VK_NULL_HANDLE;

    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkCommandBuffer> commandBuffers;
    CommandManager commandManager;

    VkSemaphore imageAvailable = VK_NULL_HANDLE;
    VkSemaphore renderFinished = VK_NULL_HANDLE;
    VkFence     inFlight       = VK_NULL_HANDLE;

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool      descriptorPool      = VK_NULL_HANDLE;
    VkDescriptorSet       descriptorSet       = VK_NULL_HANDLE;

    VkBuffer       uniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory uniformMemory = VK_NULL_HANDLE;
    void*          uniformMapped = nullptr;
    
    VkImage        depthImage     = VK_NULL_HANDLE;
    VkDeviceMemory depthMemory    = VK_NULL_HANDLE;
    VkImageView    depthImageView = VK_NULL_HANDLE;

    VkImage textureImage = VK_NULL_HANDLE;
    VkDeviceMemory textureMemory = VK_NULL_HANDLE;
    VkImageView textureImageView = VK_NULL_HANDLE;
    VkSampler textureSampler = VK_NULL_HANDLE;

    VulkanContext* ctx      = nullptr;
    Swapchain*     swapchain = nullptr;
    RendererResourceManager* resources = nullptr;

    VkBuffer       texBuffer = VK_NULL_HANDLE;
    VkDeviceMemory texMemory = VK_NULL_HANDLE;

    void init(VulkanContext* ctx, Swapchain* swapchain, RendererResourceManager* resources) {
        this->ctx       = ctx;
        this->swapchain = swapchain;
        this->resources = resources;

        createRenderPass();
        allocator.init(ctx, &commandManager);
        createDescriptorSetLayout();

        commandManager.init(ctx);
        chunkPipelineId = createChunkPipelineNew();
        uiPipelineId = createUiPipelineNew();

        createVertexBuffer();
        createDepthResources();
        createFramebuffers();
        createTextureImage();
        createTextureImageView();
        createSampler();
        createUniformBuffer();
        createDescriptorPool();
        createDescriptorSet();
        createCommandBuffers();
        createSyncObjects();
    }

    void updateUniformBuffer(const glm::mat4& view, const glm::mat4& proj) {
        UniformBufferObject ubo{};
        ubo.view = view;
        ubo.proj  = proj;
        memcpy(uniformMapped, &ubo, sizeof(ubo));
    }

    void drawFrame(World& world, RenderQueue& renderQueue) {
        vkWaitForFences(ctx->device, 1, &inFlight, VK_TRUE, UINT64_MAX);
        vkResetFences(ctx->device, 1, &inFlight);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(ctx->device, swapchain->swapchain, UINT64_MAX, imageAvailable, VK_NULL_HANDLE, &imageIndex);

        vkResetCommandBuffer(commandBuffers[imageIndex], 0);
        
        recordCommandBuffer(commandBuffers[imageIndex], imageIndex, world, renderQueue);

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submit{};
        submit.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.waitSemaphoreCount   = 1;
        submit.pWaitSemaphores      = &imageAvailable;
        submit.pWaitDstStageMask    = &waitStage;
        submit.commandBufferCount   = 1;
        submit.pCommandBuffers      = &commandBuffers[imageIndex];
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores    = &renderFinished;
        vkQueueSubmit(ctx->graphicsQueue, 1, &submit, inFlight);

        VkPresentInfoKHR present{};
        present.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores    = &renderFinished;
        present.swapchainCount     = 1;
        present.pSwapchains        = &swapchain->swapchain;
        present.pImageIndices      = &imageIndex;
        vkQueuePresentKHR(ctx->graphicsQueue, &present);
    }

    void cleanup() {
        vkDestroySampler(ctx->device, textureSampler, nullptr);
        vkDestroyImageView(ctx->device, textureImageView, nullptr);
        vkDestroyImage(ctx->device, textureImage, nullptr);
        vkFreeMemory(ctx->device, textureMemory, nullptr);
        vkDestroyImageView(ctx->device, depthImageView, nullptr);
        vkDestroyImage(ctx->device, depthImage, nullptr);
        vkFreeMemory(ctx->device, depthMemory, nullptr);
        vkDestroyBuffer(ctx->device, allocator.ssboBuffer, nullptr);
        vkFreeMemory(ctx->device, allocator.ssboMemory, nullptr);
        vkDestroyBuffer(ctx->device, uniformBuffer, nullptr);
        vkFreeMemory(ctx->device, uniformMemory, nullptr);
        vkDestroyDescriptorPool(ctx->device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(ctx->device, descriptorSetLayout, nullptr);
        vkDestroySemaphore(ctx->device, imageAvailable, nullptr);
        vkDestroySemaphore(ctx->device, renderFinished, nullptr);
        vkDestroyFence(ctx->device, inFlight, nullptr);
        commandManager.cleanup();
        for (auto fb : framebuffers) vkDestroyFramebuffer(ctx->device, fb, nullptr);
        vkDestroyRenderPass(ctx->device, renderPass, nullptr);
    }

    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboBinding{};
        uboBinding.binding         = 0;
        uboBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBinding.descriptorCount = 1;
        uboBinding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding ssboBinding{};
        ssboBinding.binding         = 1;
        ssboBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        ssboBinding.descriptorCount = 1;
        ssboBinding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding samplerBinding{};
        samplerBinding.binding         = 2;
        samplerBinding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerBinding.descriptorCount = 1;
        samplerBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding bindings[] = {uboBinding, ssboBinding, samplerBinding};

        VkDescriptorSetLayoutCreateInfo info{};
        info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 3;
        info.pBindings    = bindings;

        vkCreateDescriptorSetLayout(ctx->device, &info, nullptr, &descriptorSetLayout);
    }

    void createDescriptorPool() {
        VkDescriptorPoolSize poolSizes[3]{};
        poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = 1;
        poolSizes[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[1].descriptorCount = 1;
        poolSizes[2].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[2].descriptorCount = 1;

        VkDescriptorPoolCreateInfo info{};
        info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.poolSizeCount = 3;
        info.pPoolSizes    = poolSizes;
        info.maxSets       = 1;
        vkCreateDescriptorPool(ctx->device, &info, nullptr, &descriptorPool);
    }

    void createDescriptorSet() {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts        = &descriptorSetLayout;
        vkAllocateDescriptorSets(ctx->device, &allocInfo, &descriptorSet);

        VkDescriptorBufferInfo uboInfo{};
        uboInfo.buffer = uniformBuffer;
        uboInfo.offset = 0;
        uboInfo.range  = sizeof(UniformBufferObject);

        VkDescriptorBufferInfo ssboInfo{};
        ssboInfo.buffer = allocator.ssboBuffer;
        ssboInfo.offset = 0;
        ssboInfo.range  = allocator.ssboPoolSize;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView   = textureImageView;
        imageInfo.sampler     = textureSampler;

        VkWriteDescriptorSet writes[3]{};
        writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet          = descriptorSet;
        writes[0].dstBinding      = 0;
        writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].descriptorCount = 1;
        writes[0].pBufferInfo     = &uboInfo;

        writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet          = descriptorSet;
        writes[1].dstBinding      = 1;
        writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[1].descriptorCount = 1;
        writes[1].pBufferInfo     = &ssboInfo;

        writes[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].dstSet          = descriptorSet;
        writes[2].dstBinding      = 2;
        writes[2].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[2].descriptorCount = 1;
        writes[2].pImageInfo      = &imageInfo;

        vkUpdateDescriptorSets(ctx->device, 3, writes, 0, nullptr);
    }

    void createRenderPass() {
        VkAttachmentDescription color{};
        color.format        = swapchain->swapchainFormat;
        color.samples       = VK_SAMPLE_COUNT_1_BIT;
        color.loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color.storeOp       = VK_ATTACHMENT_STORE_OP_STORE;
        color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription depth{};
        depth.format        = VK_FORMAT_D32_SFLOAT;
        depth.samples       = VK_SAMPLE_COUNT_1_BIT;
        depth.loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth.storeOp       = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth.finalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkAttachmentReference depthRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = 1;
        subpass.pColorAttachments       = &colorRef;
        subpass.pDepthStencilAttachment = &depthRef;

        VkSubpassDependency dep{};
        dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass    = 0;
        dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkAttachmentDescription attachments[] = {color, depth};
        VkRenderPassCreateInfo  info{};
        info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = 2;
        info.pAttachments    = attachments;
        info.subpassCount    = 1;
        info.pSubpasses      = &subpass;
        info.dependencyCount = 1;
        info.pDependencies   = &dep;

        if (vkCreateRenderPass(ctx->device, &info, nullptr, &renderPass) != VK_SUCCESS)
            throw std::runtime_error("failed to create render pass");
    }

    uint32_t createChunkPipelineNew() {
        PipelineCreateInfo info{};
        info.vertPath      = "assets/shaders/chunk.vert.spv";
        info.fragPath      = "assets/shaders/chunk.frag.spv";
        info.depthTest     = true;
        info.depthWrite    = true;
        info.blend         = false;
        info.cullMode      = VK_CULL_MODE_BACK_BIT;
        info.setLayoutCount = 1;
        info.setLayout     = &descriptorSetLayout;
        info.pushSize      = sizeof(glm::mat4);
        info.renderPass    = renderPass;
        return resources->createGfxPipeline(info);
    }

    uint32_t createUiPipelineNew() {
        PipelineCreateInfo info{};
        info.vertPath = "assets/shaders/ui.vert.spv";
        info.fragPath = "assets/shaders/ui.frag.spv";
        info.depthTest = false;
        info.depthWrite = false;
        info.blend = true;
        info.cullMode = VK_CULL_MODE_NONE;

        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(UiVertex);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription attr{};
        attr.binding = 0;
        attr.location = 0;
        attr.format = VK_FORMAT_R32G32_SFLOAT; // vec3 not vec2
        attr.offset   = 0;

        info.attrCount = 1;

        info.binding = &binding;
        info.attr = &attr;

        info.setLayoutCount = 0;
        info.setLayout = nullptr;
        info.pushSize = 0;
        info.renderPass = renderPass;
        return resources->createGfxPipeline(info);
    }

    struct UiVertex {
        glm::vec2 pos;
    };

    void createVertexBuffer() {
        UiVertex positions[6] = {
            {{-0.25f, -0.25f}},
            {{ 0.25f, -0.25f}},
            {{ 0.25f,  0.25f}},
            {{-0.25f, -0.25f}},
            {{ 0.25f,  0.25f}},
            {{-0.25f,  0.25f}}
        };

        VkDeviceSize size = sizeof(positions);
        createBuffer(size,
                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     uiBuffer, uiMemory);
        void* data;
        vkMapMemory(ctx->device, uiMemory, 0, size, 0, &data);
        memcpy(data, positions, size);
        vkUnmapMemory(ctx->device, uiMemory);
    }

    void createFramebuffers() {
        framebuffers.resize(swapchain->imageViews.size());
        for (size_t i = 0; i < swapchain->imageViews.size(); i++) {
            VkImageView attachments[] = {swapchain->imageViews[i], depthImageView};
            VkFramebufferCreateInfo info{};
            info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            info.renderPass      = renderPass;
            info.attachmentCount = 2;
            info.pAttachments    = attachments;
            info.width           = swapchain->swapchainExtent.width;
            info.height          = swapchain->swapchainExtent.height;
            info.layers          = 1;
            if (vkCreateFramebuffer(ctx->device, &info, nullptr, &framebuffers[i]) != VK_SUCCESS)
                throw std::runtime_error("failed to create framebuffer");
        }
    }

    void createCommandBuffers() {
        commandBuffers.resize(framebuffers.size());
        VkCommandBufferAllocateInfo info{};
        info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.commandPool        = commandManager.commandPool;
        info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandBufferCount = (uint32_t)commandBuffers.size();
        if (vkAllocateCommandBuffers(ctx->device, &info, commandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate command buffers");
    }

    void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex, World& world, RenderQueue& renderQueue) {
        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(cmd, &begin);

        VkClearValue clearValues[2]{};
        clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo rpInfo{};
        rpInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpInfo.renderPass        = renderPass;
        rpInfo.framebuffer       = framebuffers[imageIndex];
        rpInfo.renderArea.extent = swapchain->swapchainExtent;
        rpInfo.clearValueCount   = 2;
        rpInfo.pClearValues      = clearValues;

        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

        renderQueue.flush(cmd);

        vkCmdEndRenderPass(cmd);
        vkEndCommandBuffer(cmd);
    }

    void createSyncObjects() {
        VkSemaphoreCreateInfo semInfo{};
        semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(ctx->device, &semInfo, nullptr, &imageAvailable) != VK_SUCCESS ||
            vkCreateSemaphore(ctx->device, &semInfo, nullptr, &renderFinished) != VK_SUCCESS ||
            vkCreateFence(ctx->device, &fenceInfo, nullptr, &inFlight) != VK_SUCCESS)
            throw std::runtime_error("failed to create sync objects");
    }

    // ─────────────────────────────────────────
    //  Resources
    // ─────────────────────────────────────────

    void createDepthResources() {
        createImage(swapchain->swapchainExtent.width, swapchain->swapchainExtent.height,
                    VK_FORMAT_D32_SFLOAT,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    depthImage, depthMemory);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image    = depthImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format   = VK_FORMAT_D32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(ctx->device, &viewInfo, nullptr, &depthImageView);
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

    void createUniformBuffer() {
        VkDeviceSize size = sizeof(UniformBufferObject);
        createBuffer(size,
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     uniformBuffer, uniformMemory);
        vkMapMemory(ctx->device, uniformMemory, 0, size, 0, &uniformMapped);
    }

    void createImage(uint32_t width, uint32_t height, VkFormat format,
                     VkImageTiling tiling, VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties,
                     VkImage& image, VkDeviceMemory& memory) {
        VkImageCreateInfo info{};
        info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType     = VK_IMAGE_TYPE_2D;
        info.extent        = {width, height, 1};
        info.mipLevels     = 1;
        info.arrayLayers   = 1;
        info.format        = format;
        info.tiling        = tiling;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.usage         = usage;
        info.samples       = VK_SAMPLE_COUNT_1_BIT;
        info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateImage(ctx->device, &info, nullptr, &image);

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(ctx->device, image, &memReqs);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memReqs.size;
        allocInfo.memoryTypeIndex = ctx->findMemoryType(memReqs.memoryTypeBits, properties);
        vkAllocateMemory(ctx->device, &allocInfo, nullptr, &memory);
        vkBindImageMemory(ctx->device, image, memory, 0);
    }

    void createTextureImage() {
        int w, h, channels;
        stbi_uc* pixels = stbi_load("assets/textures/texture_atlas.png", &w, &h, &channels, STBI_rgb_alpha);
        if (!pixels) throw std::runtime_error("failed to load texture");

        VkDeviceSize imageSize = w * h * 4;

        // staging buffer on CPU
        VkBuffer       stagingBuffer;
        VkDeviceMemory stagingMemory;
        createBuffer(imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingMemory);

        void* data;
        vkMapMemory(ctx->device, stagingMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, imageSize);
        vkUnmapMemory(ctx->device, stagingMemory);
        stbi_image_free(pixels);

        // create GPU image
        createImage(w, h,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            textureImage, textureMemory);

        // transition + copy + transition
        transitionImageLayout(textureImage,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer, textureImage, w, h);
        transitionImageLayout(textureImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(ctx->device, stagingBuffer, nullptr);
        vkFreeMemory(ctx->device, stagingMemory, nullptr);
    }

    void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer cmd = commandManager.beginOneShot();

        VkImageMemoryBarrier barrier{};
        barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout           = oldLayout;
        barrier.newLayout           = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image               = image;
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.layerCount     = 1;

        VkPipelineStageFlags srcStage, dstStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
            newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }

        vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        commandManager.endOneShot(cmd);
    }

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t w, uint32_t h) {
        VkCommandBuffer cmd = commandManager.beginOneShot();

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent                 = {w, h, 1};

        vkCmdCopyBufferToImage(cmd, buffer, image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        commandManager.endOneShot(cmd);
    }

    void createTextureImageView() {
        VkImageViewCreateInfo info{};
        info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image    = textureImage;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format   = VK_FORMAT_R8G8B8A8_SRGB;
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.layerCount = 1;
        vkCreateImageView(ctx->device, &info, nullptr, &textureImageView);
    }

    void createSampler() {
        VkSamplerCreateInfo info{};
        info.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.magFilter    = VK_FILTER_NEAREST; // pixel art / voxel style
        info.minFilter    = VK_FILTER_NEAREST;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vkCreateSampler(ctx->device, &info, nullptr, &textureSampler);
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo info{};
        info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        info.codeSize = code.size();
        info.pCode    = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule mod;
        vkCreateShaderModule(ctx->device, &info, nullptr, &mod);
        return mod;
    }

    static std::vector<char> readFile(const std::string& path) {
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) throw std::runtime_error("failed to open: " + path);
        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        rewind(f);
        std::vector<char> buf(size);
        fread(buf.data(), 1, size, f);
        fclose(f);
        return buf;
    }
};