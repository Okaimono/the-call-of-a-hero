#define GLFW_INCLUDE_VULKAN
#define STB_IMAGE_IMPLEMENTATION
#include "vulkan_includes.hpp"
#include "stb_image.h"
#include "core/types.hpp"
#include "core/config.hpp"
#include "game/camera.hpp"
#include "world/world.hpp"
#include "vulkan/vulkan_context.hpp"
#include "vulkan/swapchain.hpp"

class App {
public:
    void run() {
        initWindow();
        initVulkan();
        loop();
        cleanup();
    }

private:
    GLFWwindow* window = nullptr;
    VulkanContext ctx;
    Swapchain swapchain;
    World world;
    Camera camera;

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    VkRenderPass renderPass       = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline           = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkCommandBuffer> commandBuffers;
    VkSemaphore imageAvailable    = VK_NULL_HANDLE;
    VkSemaphore renderFinished    = VK_NULL_HANDLE;
    VkFence inFlight              = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

    Buffer uniformBuffer;

    void* uniformMapped           = nullptr;
    VkImage textureImage          = VK_NULL_HANDLE;
    VkDeviceMemory textureMemory  = VK_NULL_HANDLE;
    VkImageView textureImageView  = VK_NULL_HANDLE;
    VkSampler textureSampler      = VK_NULL_HANDLE;

    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(Config::WIDTH, Config::HEIGHT, "coah-engine", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetCursorPosCallback(window, mouseCallback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    void initVulkan() {
        world.generateWorld(Config::RENDER_RADIUS);
        ctx.init(window);
        swapchain.init(ctx);
        createDescriptorSetLayout();
        createRenderPass();
        createPipeline();
        createFramebuffers();
        createTexture();
        uploadChunkMeshes();
        uploadEntityMesh();
        createCommandBuffers();
        createSyncObjects();
        createUniformBuffer();
        createDescriptorPool();
        createDescriptorSet();
    }

    void loop() {
        while (!glfwWindowShouldClose(window)) {
            float currentFrame = glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            glfwPollEvents();
            inputs();
            moveEntity();
            updateUniformBuffer();
            drawFrame();
        }
        vkDeviceWaitIdle(ctx.device);
    }

    void moveEntity() {
        static float timer = 0.0f;
        timer += deltaTime;
        
        if (timer >= 1.0f) {
            timer = 0.0f;
            world.entity.walk(Direction::NORTH);
            uploadEntityMesh();
        }
    }

    void inputs() {
        float velocity = camera.speed * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.position += velocity * camera.front;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.position -= velocity * camera.front;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.position -= glm::normalize(glm::cross(camera.front, camera.up)) * velocity;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.position += glm::normalize(glm::cross(camera.front, camera.up)) * velocity;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) camera.position += camera.up * velocity;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camera.position -= camera.up * velocity;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    }

    static void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
        App* app = (App*)glfwGetWindowUserPointer(window);
        Camera& cam = app->camera;
        if (cam.firstMouse) { cam.lastX = xpos; cam.lastY = ypos; cam.firstMouse = false; }
        float xoffset = (xpos - cam.lastX) * cam.sensitivity;
        float yoffset = (cam.lastY - ypos) * cam.sensitivity;
        cam.lastX = xpos; cam.lastY = ypos;
        cam.yaw += xoffset; cam.pitch += yoffset;
        cam.pitch = glm::clamp(cam.pitch, -89.0f, 89.0f);
        glm::vec3 dir;
        dir.x = cos(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
        dir.y = sin(glm::radians(cam.pitch));
        dir.z = sin(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
        cam.front = glm::normalize(dir);
    }

    void createRenderPass() {
        VkAttachmentDescription color{};
        color.format = swapchain.format;
        color.samples = VK_SAMPLE_COUNT_1_BIT;
        color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription depth{};
        depth.format = VK_FORMAT_D32_SFLOAT;
        depth.samples = VK_SAMPLE_COUNT_1_BIT;
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorRef{};
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthRef{};
        depthRef.attachment = 1;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;
        subpass.pDepthStencilAttachment = &depthRef;

        VkSubpassDependency dep{};
        dep.srcSubpass = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass = 0;
        dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkAttachmentDescription attachments[] = {color, depth};
        VkRenderPassCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = 2;
        info.pAttachments = attachments;
        info.subpassCount = 1;
        info.pSubpasses = &subpass;
        info.dependencyCount = 1;
        info.pDependencies = &dep;

        if (vkCreateRenderPass(ctx.device, &info, nullptr, &renderPass) != VK_SUCCESS)
            throw std::runtime_error("failed to create render pass");
    }

    void createPipeline() {
        auto vert = readFile("assets/shaders/vert.spv");
        auto frag = readFile("assets/shaders/frag.spv");
        VkShaderModule vertMod = createShaderModule(vert);
        VkShaderModule fragMod = createShaderModule(frag);

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertMod;
        stages[0].pName = "main";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragMod;
        stages[1].pName = "main";

        VkVertexInputBindingDescription bindingDesc{};
        bindingDesc.binding = 0;
        bindingDesc.stride = sizeof(Vertex);
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription attrDescs[3]{};
        attrDescs[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)};
        attrDescs[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)};
        attrDescs[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, uv)};

        VkPipelineVertexInputStateCreateInfo vertInput{};
        vertInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertInput.vertexBindingDescriptionCount = 1;
        vertInput.pVertexBindingDescriptions = &bindingDesc;
        vertInput.vertexAttributeDescriptionCount = 3;
        vertInput.pVertexAttributeDescriptions = attrDescs;

        VkPipelineInputAssemblyStateCreateInfo assembly{};
        assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkViewport viewport{};
        viewport.width = (float)swapchain.extent.width;
        viewport.height = (float)swapchain.extent.height;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{};
        scissor.extent = swapchain.extent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo raster{};
        raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode = VK_CULL_MODE_NONE;
        raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        raster.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo ms{};
        ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState blendAttach{};
        blendAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo blend{};
        blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blend.attachmentCount = 1;
        blend.pAttachments = &blendAttach;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

        VkPushConstantRange pushRange{};
        pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushRange.offset = 0;
        pushRange.size = sizeof(glm::mat4);

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = &descriptorSetLayout;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushRange;
        vkCreatePipelineLayout(ctx.device, &layoutInfo, nullptr, &pipelineLayout);

        VkGraphicsPipelineCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        info.stageCount = 2;
        info.pStages = stages;
        info.pVertexInputState = &vertInput;
        info.pInputAssemblyState = &assembly;
        info.pViewportState = &viewportState;
        info.pRasterizationState = &raster;
        info.pDepthStencilState = &depthStencil;
        info.pMultisampleState = &ms;
        info.pColorBlendState = &blend;
        info.layout = pipelineLayout;
        info.renderPass = renderPass;

        if (vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline) != VK_SUCCESS)
            throw std::runtime_error("failed to create pipeline");

        vkDestroyShaderModule(ctx.device, vertMod, nullptr);
        vkDestroyShaderModule(ctx.device, fragMod, nullptr);
        std::cout << "pipeline created" << std::endl;
    }

    void createFramebuffers() {
        framebuffers.resize(swapchain.imageViews.size());
        for (size_t i = 0; i < swapchain.imageViews.size(); i++) {
            VkImageView attachments[] = { swapchain.imageViews[i], swapchain.depthImageView };
            VkFramebufferCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            info.renderPass = renderPass;
            info.attachmentCount = 2;
            info.pAttachments = attachments;
            info.width = swapchain.extent.width;
            info.height = swapchain.extent.height;
            info.layers = 1;
            if (vkCreateFramebuffer(ctx.device, &info, nullptr, &framebuffers[i]) != VK_SUCCESS)
                throw std::runtime_error("failed to create framebuffer");
        }
    }

    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboBinding{};
        uboBinding.binding = 0;
        uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBinding.descriptorCount = 1;
        uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding samplerBinding{};
        samplerBinding.binding = 1;
        samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerBinding.descriptorCount = 1;
        samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding bindings[] = {uboBinding, samplerBinding};
        VkDescriptorSetLayoutCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 2;
        info.pBindings = bindings;
        vkCreateDescriptorSetLayout(ctx.device, &info, nullptr, &descriptorSetLayout);
    }

    void createUniformBuffer() {
        VkDeviceSize size = sizeof(UniformBufferObject);
        uniformBuffer.createBuffer(ctx, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vkMapMemory(ctx.device, uniformBuffer.memory, 0, size, 0, &uniformMapped);
    }

    void createDescriptorPool() {
        VkDescriptorPoolSize poolSizes[2]{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = 1;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = 1;
        VkDescriptorPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.poolSizeCount = 2;
        info.pPoolSizes = poolSizes;
        info.maxSets = 1;
        vkCreateDescriptorPool(ctx.device, &info, nullptr, &descriptorPool);
    }

    void createDescriptorSet() {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;
        vkAllocateDescriptorSets(ctx.device, &allocInfo, &descriptorSet);

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffer.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = textureSampler;

        VkWriteDescriptorSet writes[2]{};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = descriptorSet;
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].descriptorCount = 1;
        writes[0].pBufferInfo = &bufferInfo;
        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = descriptorSet;
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo = &imageInfo;
        vkUpdateDescriptorSets(ctx.device, 2, writes, 0, nullptr);
    }

    void updateUniformBuffer() {
        UniformBufferObject ubo{};
        ubo.model = glm::mat4(1.0f);
        ubo.view = glm::lookAt(camera.position, camera.position + camera.front, camera.up);
        ubo.proj = glm::perspective(glm::radians(Config::FOV),
            Config::WIDTH / (float)Config::HEIGHT, Config::NEAR, Config::FAR);
        ubo.proj[1][1] *= -1;
        memcpy(uniformMapped, &ubo, sizeof(ubo));
    }

    void uploadEntityMesh() {
        Entity& entity = world.entity;
        ChunkMesh mesh = entity.createMesh();
        VkDeviceSize size = sizeof(mesh.vertices[0]) * mesh.vertices.size();

        Buffer staging;
        staging.createBuffer(ctx, size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void* data;
        vkMapMemory(ctx.device, staging.memory, 0, size, 0, &data);
        memcpy(data, mesh.vertices.data(), size);
        vkUnmapMemory(ctx.device, staging.memory);

        entity.vertexBuffer.createBuffer(ctx, size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkCommandBuffer cmd = ctx.beginOneTimeCommands();
        VkBufferCopy copy{};
        copy.size = size;
        vkCmdCopyBuffer(cmd, staging.buffer, entity.vertexBuffer.buffer, 1, &copy);
        ctx.endOneTimeCommands(cmd);

        staging.deleteBuffer(ctx);
        entity.vertexCount = mesh.vertices.size();
    }

    void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer cmd = ctx.beginOneTimeCommands();
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;
        VkPipelineStageFlags srcStage, dstStage;
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
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
        ctx.endOneTimeCommands(cmd);
    }

    void createTexture() {
        int w, h, channels;
        stbi_uc* pixels = stbi_load("assets/textures/cobblestone.png", &w, &h, &channels, STBI_rgb_alpha);
        if (!pixels) throw std::runtime_error("failed to load texture");
        VkDeviceSize size = w * h * 4;

        Buffer stagingBuffer;
        stagingBuffer.createBuffer(ctx, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        void* data;
        vkMapMemory(ctx.device, stagingBuffer.memory, 0, size, 0, &data);
        memcpy(data, pixels, size);
        vkUnmapMemory(ctx.device, stagingBuffer.memory);
        stbi_image_free(pixels);

        VkImageCreateInfo imgInfo{};
        imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imgInfo.imageType = VK_IMAGE_TYPE_2D;
        imgInfo.extent = {(uint32_t)w, (uint32_t)h, 1};
        imgInfo.mipLevels = 1; imgInfo.arrayLayers = 1;
        imgInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateImage(ctx.device, &imgInfo, nullptr, &textureImage);

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(ctx.device, textureImage, &memReqs);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = ctx.findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(ctx.device, &allocInfo, nullptr, &textureMemory);
        vkBindImageMemory(ctx.device, textureImage, textureMemory, 0);

        transitionImageLayout(textureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        VkCommandBuffer cmd = ctx.beginOneTimeCommands();
        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {(uint32_t)w, (uint32_t)h, 1};
        vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        ctx.endOneTimeCommands(cmd);
        transitionImageLayout(textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        stagingBuffer.deleteBuffer(ctx);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = textureImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(ctx.device, &viewInfo, nullptr, &textureImageView);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        vkCreateSampler(ctx.device, &samplerInfo, nullptr, &textureSampler);
    }

    void uploadChunkMeshes() {
        for (auto& [coord, chunk] : world.chunks) {

            ChunkMesh mesh = chunk.createMesh(coord);
            if (mesh.vertices.empty()) continue;

            VkDeviceSize size = sizeof(mesh.vertices[0]) * mesh.vertices.size();

            Buffer stagingBuffer;
            stagingBuffer.createBuffer(ctx, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            void* data;
            vkMapMemory(ctx.device, stagingBuffer.memory, 0, size, 0, &data);
            memcpy(data, mesh.vertices.data(), size);
            vkUnmapMemory(ctx.device, stagingBuffer.memory);

            chunk.vertexBuffer.createBuffer(ctx, size,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            VkCommandBuffer cmd = ctx.beginOneTimeCommands();
            VkBufferCopy copy{};
            copy.size = size;
            vkCmdCopyBuffer(cmd, stagingBuffer.buffer, chunk.vertexBuffer.buffer, 1, &copy);
            ctx.endOneTimeCommands(cmd);

            stagingBuffer.deleteBuffer(ctx);
            chunk.vertexCount = mesh.vertices.size();
        }
    }

    void createCommandBuffers() {
        commandBuffers.resize(framebuffers.size());
        VkCommandBufferAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.commandPool = ctx.commandPool;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandBufferCount = commandBuffers.size();
        if (vkAllocateCommandBuffers(ctx.device, &info, commandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate command buffers");
    }

    void createSyncObjects() {
        VkSemaphoreCreateInfo semInfo{};
        semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateSemaphore(ctx.device, &semInfo, nullptr, &imageAvailable);
        vkCreateSemaphore(ctx.device, &semInfo, nullptr, &renderFinished);
        vkCreateFence(ctx.device, &fenceInfo, nullptr, &inFlight);
    }

    void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex) {
        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(cmd, &begin);

        VkClearValue clearValues[2]{};
        clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo rpInfo{};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpInfo.renderPass = renderPass;
        rpInfo.framebuffer = framebuffers[imageIndex];
        rpInfo.renderArea.extent = swapchain.extent;
        rpInfo.clearValueCount = 2;
        rpInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        glm::mat4 identity = glm::mat4(1.0f);
        vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &identity);

        for (auto& [coord, chunk] : world.chunks) {
            if (chunk.vertexCount == 0) continue;
            VkBuffer vbs[] = {chunk.vertexBuffer.buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cmd, 0, 1, vbs, offsets);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
            vkCmdDraw(cmd, chunk.vertexCount, 1, 0, 0);
        }

        glm::mat4 entityModel = glm::translate(glm::mat4(1.0f), world.entity.position);
        vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &entityModel);
        VkBuffer entityVbs[] = {world.entity.vertexBuffer.buffer};
        VkDeviceSize entityOffsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, entityVbs, entityOffsets);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
        vkCmdDraw(cmd, world.entity.vertexCount, 1, 0, 0);

        vkCmdEndRenderPass(cmd);
        vkEndCommandBuffer(cmd);
    }

    void drawFrame() {
        vkWaitForFences(ctx.device, 1, &inFlight, VK_TRUE, UINT64_MAX);
        vkResetFences(ctx.device, 1, &inFlight);
        uint32_t imageIndex;
        vkAcquireNextImageKHR(ctx.device, swapchain.swapchain, UINT64_MAX, imageAvailable, VK_NULL_HANDLE, &imageIndex);
        vkResetCommandBuffer(commandBuffers[imageIndex], 0);
        recordCommandBuffer(commandBuffers[imageIndex], imageIndex);

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &imageAvailable;
        submit.pWaitDstStageMask = &waitStage;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &commandBuffers[imageIndex];
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &renderFinished;
        vkQueueSubmit(ctx.graphicsQueue, 1, &submit, inFlight);

        VkPresentInfoKHR present{};
        present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = &renderFinished;
        present.swapchainCount = 1;
        present.pSwapchains = &swapchain.swapchain;
        present.pImageIndices = &imageIndex;
        vkQueuePresentKHR(ctx.graphicsQueue, &present);
    }

    static std::vector<char> readFile(const std::string& path) {
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) throw std::runtime_error("failed to open: " + path);
        fseek(f, 0, SEEK_END);
        size_t size = ftell(f); rewind(f);
        std::vector<char> buf(size);
        fread(buf.data(), 1, size, f);
        fclose(f);
        return buf;
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        info.codeSize = code.size();
        info.pCode = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule mod;
        vkCreateShaderModule(ctx.device, &info, nullptr, &mod);
        return mod;
    }

    void cleanup() {
        vkDestroySampler(ctx.device, textureSampler, nullptr);
        vkDestroyImageView(ctx.device, textureImageView, nullptr);
        vkDestroyImage(ctx.device, textureImage, nullptr);
        vkFreeMemory(ctx.device, textureMemory, nullptr);
        for (auto& [coord, chunk] : world.chunks) {
            chunk.vertexBuffer.deleteBuffer(ctx);
        }
        uniformBuffer.deleteBuffer(ctx);
        vkDestroyDescriptorPool(ctx.device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(ctx.device, descriptorSetLayout, nullptr);
        vkDestroySemaphore(ctx.device, imageAvailable, nullptr);
        vkDestroySemaphore(ctx.device, renderFinished, nullptr);
        vkDestroyFence(ctx.device, inFlight, nullptr);
        for (auto fb : framebuffers) vkDestroyFramebuffer(ctx.device, fb, nullptr);
        vkDestroyPipeline(ctx.device, pipeline, nullptr);
        vkDestroyPipelineLayout(ctx.device, pipelineLayout, nullptr);
        vkDestroyRenderPass(ctx.device, renderPass, nullptr);
        swapchain.cleanup(ctx);
        ctx.cleanup();
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main() {
    App app;
    try { app.run(); }
    catch (const std::exception& e) { std::cerr << e.what() << std::endl; return 1; }
    return 0;
}