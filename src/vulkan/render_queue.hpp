#pragma once
#include "vulkan_includes.hpp"

struct DrawCall {
    VkPipeline pipeline;
    VkPipelineLayout layout;
    VkDescriptorSet descriptorSet;
    VkBuffer vertexBuffer;
    bool pushConstant = false;
    size_t pushSize = 0;
    void* pushData = nullptr;
    glm::mat4 model = glm::mat4(1.0f);
    uint32_t vertexCount;
    uint32_t firstVertex;
};

struct RenderQueue {
    std::vector<DrawCall> calls;
    void push(DrawCall call) { calls.push_back(call); }
    void clear()             { calls.clear(); }

    void flush(VkCommandBuffer cmd) {
        VkPipeline lastPipeline = VK_NULL_HANDLE;
        VkDescriptorSet lastDescriptorSet = VK_NULL_HANDLE;
        VkPipelineLayout lastLayout = VK_NULL_HANDLE;
        VkBuffer lastVertexBuffer = VK_NULL_HANDLE;

        for (auto& call : calls) {
            if (call.pipeline != lastPipeline) {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, call.pipeline);
                lastPipeline = call.pipeline;
            }
            if (call.descriptorSet != VK_NULL_HANDLE && 
                (call.descriptorSet != lastDescriptorSet || call.layout != lastLayout)) {
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    call.layout, 0, 1, &call.descriptorSet, 0, nullptr);
                lastDescriptorSet = call.descriptorSet;
                lastLayout = call.layout;
            }
            if (call.descriptorSet != VK_NULL_HANDLE && call.descriptorSet != lastDescriptorSet) {
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    call.layout, 0, 1, &call.descriptorSet, 0, nullptr);
                lastDescriptorSet = call.descriptorSet;
            }
            if (call.vertexBuffer != VK_NULL_HANDLE && call.vertexBuffer != lastVertexBuffer) {
                VkDeviceSize offset = 0;
                vkCmdBindVertexBuffers(cmd, 0, 1, &call.vertexBuffer, &offset);
                lastVertexBuffer = call.vertexBuffer;
            }
            if (call.pushConstant) {
                if (call.pushData == nullptr) {
                    vkCmdPushConstants(cmd, call.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &call.model);
                } else {
                    vkCmdPushConstants(cmd, call.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, call.pushSize, call.pushData);
                }
            }
            vkCmdDraw(cmd, call.vertexCount, 1, 0, call.firstVertex);
        }
    }
};