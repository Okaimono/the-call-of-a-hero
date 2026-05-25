#pragma once
#include "vulkan/vulkan_context.hpp"
#include "vulkan/swapchain.hpp"
#include <stdexcept>
#include <vector>
#include <string>
#include <cstdio>

struct Handle { uint32_t index; };

struct PipelineCreateInfo {
    const char* vertPath;
    const char* fragPath;

    bool depthTest  = true;
    bool depthWrite = true;
    bool blend      = false;
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;

    VkVertexInputBindingDescription*   binding   = nullptr;
    VkVertexInputAttributeDescription* attr      = nullptr;
    uint32_t                           attrCount = 0;

    uint32_t               setLayoutCount = 0;
    VkDescriptorSetLayout* setLayout      = nullptr;

    uint32_t     pushSize   = 0;
    VkRenderPass renderPass = VK_NULL_HANDLE;
};

struct GfxPipeline {
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
};

class RendererResourceManager {
public:
    void init(VulkanContext* ctx, Swapchain* swapchain) {
        this->ctx = ctx;
        this->swapchain = swapchain;
    }

    uint32_t createGfxPipeline(PipelineCreateInfo& info) {
        GfxPipeline gfxPipeline{};

        auto vert = readFile(info.vertPath);
        auto frag = readFile(info.fragPath);
        VkShaderModule vertMod = createShaderModule(vert);
        VkShaderModule fragMod = createShaderModule(frag);

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertMod;
        stages[0].pName  = "main";
        stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragMod;
        stages[1].pName  = "main";

        VkPipelineVertexInputStateCreateInfo vertInput{};
        vertInput.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        if (info.attrCount != 0) {
            vertInput.vertexBindingDescriptionCount   = 1;
            vertInput.pVertexBindingDescriptions = info.binding;
            vertInput.vertexAttributeDescriptionCount = info.attrCount;
            vertInput.pVertexAttributeDescriptions    = info.attr;
        }

        VkPipelineInputAssemblyStateCreateInfo assembly{};
        assembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkViewport viewport{};
        viewport.width    = (float)swapchain->swapchainExtent.width;
        viewport.height   = (float)swapchain->swapchainExtent.height;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.extent = swapchain->swapchainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports    = &viewport;
        viewportState.scissorCount  = 1;
        viewportState.pScissors     = &scissor;

        VkPipelineRasterizationStateCreateInfo raster{};
        raster.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode    = info.cullMode;
        raster.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        raster.lineWidth   = 1.0f;

        VkPipelineMultisampleStateCreateInfo ms{};
        ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable  = info.depthTest  ? VK_TRUE : VK_FALSE;
        depthStencil.depthWriteEnable = info.depthWrite ? VK_TRUE : VK_FALSE;
        depthStencil.depthCompareOp   = VK_COMPARE_OP_LESS;

        VkPipelineColorBlendAttachmentState blendAttach{};
        blendAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blendAttach.blendEnable = info.blend ? VK_TRUE : VK_FALSE;

        if (info.blend) {
            blendAttach.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            blendAttach.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blendAttach.colorBlendOp        = VK_BLEND_OP_ADD;
        }

        VkPushConstantRange pushRange{};
        pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushRange.offset     = 0;
        pushRange.size       = info.pushSize;

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount         = info.setLayoutCount;
        layoutInfo.pSetLayouts            = info.setLayout;
        layoutInfo.pushConstantRangeCount = info.pushSize > 0 ? 1 : 0;
        layoutInfo.pPushConstantRanges    = info.pushSize > 0 ? &pushRange : nullptr;
        vkCreatePipelineLayout(ctx->device, &layoutInfo, nullptr, &gfxPipeline.layout);

        VkPipelineColorBlendStateCreateInfo blendState{};
        blendState.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blendState.attachmentCount = 1;
        blendState.pAttachments    = &blendAttach;

        VkGraphicsPipelineCreateInfo createInfo{};
        createInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        createInfo.stageCount          = 2;
        createInfo.pStages             = stages;
        createInfo.pVertexInputState   = &vertInput;
        createInfo.pInputAssemblyState = &assembly;
        createInfo.pViewportState      = &viewportState;
        createInfo.pRasterizationState = &raster;
        createInfo.pDepthStencilState  = &depthStencil;
        createInfo.pMultisampleState   = &ms;
        createInfo.pColorBlendState    = &blendState;
        createInfo.layout              = gfxPipeline.layout;
        createInfo.renderPass          = info.renderPass;

        printf("setLayoutCount: %u  pushSize: %u\n", info.setLayoutCount, info.pushSize);
        if (vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &gfxPipeline.pipeline) != VK_SUCCESS)
            throw std::runtime_error("failed to create pipeline");

        vkDestroyShaderModule(ctx->device, vertMod, nullptr);
        vkDestroyShaderModule(ctx->device, fragMod, nullptr);

        gfxPipelines.push_back(gfxPipeline);
        std::cout << "pipeline created\n";
        return { (uint32_t)gfxPipelines.size() - 1 };
    }

    GfxPipeline& getPipeline(uint32_t id) {
        return gfxPipelines[id];
    }

    void cleanup() {
        for (auto& p : gfxPipelines) {
            vkDestroyPipeline(ctx->device, p.pipeline, nullptr);
            vkDestroyPipelineLayout(ctx->device, p.layout, nullptr);
        }
    }

private:
    VulkanContext* ctx = nullptr;
    Swapchain* swapchain = nullptr;

    std::vector<GfxPipeline> gfxPipelines;

    static std::vector<char> readFile(const char* path) {
        FILE* f = fopen(path, "rb");
        if (!f) throw std::runtime_error(std::string("failed to open: ") + path);
        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        rewind(f);
        std::vector<char> buf(size);
        fread(buf.data(), 1, size, f);
        fclose(f);
        return buf;
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
};