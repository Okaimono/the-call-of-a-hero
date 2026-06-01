#pragma once
#include "vulkan/renderer.hpp"
#include "vulkan/renderer_resource_manager.hpp"
#include <functional>

#include <cstdio>
#include <math.h>
#include <cmath>

struct BasiliskVertex {
    glm::vec3 pos;
    glm::vec3 color;
};

struct BasiliskPush {
    glm::mat4 model = glm::mat4(1.0f);
    glm::vec3 color = glm::vec3(0.05f, 0.05f, 0.07f);
};

struct Segment {
    glm::vec3 pos;
    BasiliskPush push;
};

struct Head {
    std::vector<Segment> headSegments;
};

class Basilisk {
public:
    Renderer* renderer = nullptr;
    RendererResourceManager* rResourceManager = nullptr;

    uint32_t gfxPipeline = 0;

    VkBuffer       buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;

    std::vector<Segment> segments;
    Head head;

    // Velocity of the head of basilisk
    glm::vec3 headVelocity = glm::vec3(2.0f, 0.0f, 2.0f);

    glm::vec3 segmentDist = glm::vec3(1.0f, 0.0f, 0.0f);

    void init(Renderer* renderer, RendererResourceManager* rResourceManager) {
        this->renderer = renderer;
        this->rResourceManager = rResourceManager;
        initPipeline();
        createHead();
        createBody();
        createVertexBuffer();
    }

    void initPipeline() {
        PipelineCreateInfo info{};
        info.vertPath = "assets/shaders/basilisk.vert.spv";
        info.fragPath = "assets/shaders/basilisk.frag.spv";

        info.depthTest = true;
        info.depthWrite = true;
        info.blend = false;
        info.cullMode = VK_CULL_MODE_NONE;

        VkVertexInputBindingDescription binding{};
        binding.binding   = 0;
        binding.stride    = sizeof(BasiliskVertex);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription attr{};
        attr.binding  = 0;
        attr.location = 0;
        attr.format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 not vec2
        attr.offset   = 0;
        
        info.binding = &binding;
        info.attr = &attr;
        info.attrCount = 1;

        info.setLayoutCount = 1;
        info.setLayout = &renderer->descriptorSetLayout;

        info.pushSize = sizeof(BasiliskPush);
        info.renderPass = renderer->renderPass;

        gfxPipeline = rResourceManager->createGfxPipeline(info);
    }

    void update(float dt, const glm::vec3& playerPos) {
        updateVelocity(dt);
        updatePos(dt);
        rotateToPlayer(dt, playerPos);
    }

    void updateVelocity(float dt) {
        static float start = 0.0f;
        float oldStart = start;
        start += dt;
        headVelocity.y = cos(start) * 3;

        if (ceil(oldStart) == ceil(start)) {return; }
    }

    void rotateToPlayer(float dt, const glm::vec3& playerPos) {
        glm::vec3 tempPlayerPos = playerPos;
        tempPlayerPos.y = 0;
        glm::vec3 tempHeadPos = segments[0].pos;
        tempHeadPos.y = 0;

        glm::vec3 vec = glm::normalize(tempPlayerPos - tempHeadPos);
        vec.y = 0.0f;

        glm::vec3 tempVelocity = headVelocity;
        tempVelocity.y = 0;

        float length = glm::length(tempVelocity);
        vec *= length;

        headVelocity.x = vec.x;
        headVelocity.z = vec.z;
    }

    void updatePos(float dt) {
        if (glm::length(headVelocity) == 0) {return;}

        float speed = 1.0f;

        glm::vec3 d_headPos = dt * headVelocity;
        segments[0].pos = segments[0].pos + d_headPos;

        // Segment 0 is the head.
        for (int i = 1; i < segments.size(); i++) {
            // Distance between the segment and segment in front of it
            
            glm::vec3 dist = segments[i - 1].pos - segments[i].pos;
            glm::vec3 move = glm::normalize(dist);
            segments[i].pos = segments[i - 1].pos - move;

            glm::vec3 forward = glm::vec3(0.0f, 0.0f, 1.0f);
            glm::vec3 dir = glm::normalize(dist);
            
            float yaw = atan2(dir.x, dir.z);
            float pitch = asin(dir.y);

            // Linear algebra for the actual mesh
            glm::mat4 model = glm::mat4(1.0f);

            // 2. Apply Translation first (Standard TRS order: Translate * Rotate * Scale)
            model = glm::translate(model, segments[i].pos);

            // 3. Apply Yaw (Rotation around the world Up vector)
            model = glm::rotate(model, yaw, glm::vec3(0.0f, 1.0f, 0.0f));

            // 4. Apply Pitch (Rotation around the LOCAL X-axis)
            model = glm::rotate(model, -pitch, glm::vec3(1.0f, 0.0f, 0.0f));
            segments[i].push.model = model;
        }
    }

    struct BlocksInfo {
        std::vector<glm::vec3> blocks;
        glm::vec3 color;
    };

    void createHead() {
        std::vector<glm::vec3> headBlocks;
        std::vector<Segment>& headSegments = head.headSegments;

        BlocksInfo blackBlocks;
        blackBlocks.color = glm::vec3(0.05f, 0.05f, 0.07f);

        // Bottom row
        blackBlocks.blocks.push_back(glm::vec3(-4.0f, 0.0f, 3.0f));
        blackBlocks.blocks.push_back(glm::vec3(-3.0f, 0.0f, 2.0f));
        blackBlocks.blocks.push_back(glm::vec3(-2.0f, 0.0f, 1.0f));
        blackBlocks.blocks.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
        blackBlocks.blocks.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
        blackBlocks.blocks.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
        blackBlocks.blocks.push_back(glm::vec3(2.0f, 0.0f, 1.0f));
        blackBlocks.blocks.push_back(glm::vec3(3.0f, 0.0f, 2.0f));
        blackBlocks.blocks.push_back(glm::vec3(4.0f, 0.0f, 3.0f));

        // 3rd row
        blackBlocks.blocks.push_back(glm::vec3(-4.0f, 2.0f, 3.0f));
        blackBlocks.blocks.push_back(glm::vec3(-3.0f, 2.0f, 2.0f));
        blackBlocks.blocks.push_back(glm::vec3(-2.0f, 2.0f, 1.0f));
        blackBlocks.blocks.push_back(glm::vec3(-1.0f, 2.0f, 0.0f));
        blackBlocks.blocks.push_back(glm::vec3(0.0f, 2.0f, 0.0f));
        blackBlocks.blocks.push_back(glm::vec3(1.0f, 2.0f, 0.0f));
        blackBlocks.blocks.push_back(glm::vec3(2.0f, 2.0f, 1.0f));
        blackBlocks.blocks.push_back(glm::vec3(3.0f, 2.0f, 2.0f));
        blackBlocks.blocks.push_back(glm::vec3(4.0f, 2.0f, 3.0f));

        // Top row
        blackBlocks.blocks.push_back(glm::vec3(-3.0f, 3.0f, 3.0f));
        blackBlocks.blocks.push_back(glm::vec3(-2.0f, 3.0f, 2.0f));
        blackBlocks.blocks.push_back(glm::vec3(-1.0f, 3.0f, 1.0f));
        blackBlocks.blocks.push_back(glm::vec3(0.0f, 3.0f, 1.0f));
        blackBlocks.blocks.push_back(glm::vec3(1.0f, 3.0f, 1.0f));
        blackBlocks.blocks.push_back(glm::vec3(2.0f, 3.0f, 2.0f));
        blackBlocks.blocks.push_back(glm::vec3(3.0f, 3.0f, 3.0f));

        // Right/left face
        blackBlocks.blocks.push_back(glm::vec3(-5.0f, 3.0f, 4.0f));
        blackBlocks.blocks.push_back(glm::vec3(-5.0f, 2.0f, 4.0f));
        blackBlocks.blocks.push_back(glm::vec3(-5.0f, 1.0f, 4.0f));

        blackBlocks.blocks.push_back(glm::vec3(5.0f, 3.0f, 4.0f));
        blackBlocks.blocks.push_back(glm::vec3(5.0f, 2.0f, 4.0f));
        blackBlocks.blocks.push_back(glm::vec3(5.0f, 1.0f, 4.0f));

        for (float i = -2; i <= 2; i++) {
            blackBlocks.blocks.push_back(glm::vec3(i, 3.0f, 2.0f));
        }
        
        for (float i = -3; i <= 3; i++) {
            blackBlocks.blocks.push_back(glm::vec3(i, 3.0f, 3.0f));
        }

        BlocksInfo greenBlocks;
        greenBlocks.color = glm::vec3(0.0f, 0.8f, 0.1f);

        // 2nd row
        greenBlocks.blocks.push_back(glm::vec3(-4.0f, 1.0f, 3.0f));
        greenBlocks.blocks.push_back(glm::vec3(-3.0f, 1.0f, 2.0f));
        greenBlocks.blocks.push_back(glm::vec3(-2.0f, 1.0f, 1.0f));
        greenBlocks.blocks.push_back(glm::vec3(-1.0f, 1.0f, 0.0f));
        greenBlocks.blocks.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
        greenBlocks.blocks.push_back(glm::vec3(1.0f, 1.0f, 0.0f));
        greenBlocks.blocks.push_back(glm::vec3(2.0f, 1.0f, 1.0f));
        greenBlocks.blocks.push_back(glm::vec3(3.0f, 1.0f, 2.0f));
        greenBlocks.blocks.push_back(glm::vec3(4.0f, 1.0f, 3.0f));

        // Eyes
        greenBlocks.blocks.push_back(glm::vec3(-4.0f, 3.0f, 3.0f));
        greenBlocks.blocks.push_back(glm::vec3(4.0f, 3.0f, 3.0f));

        createBlocks(blackBlocks, headSegments);
        createBlocks(greenBlocks, headSegments);
    }

    void createBlocks(BlocksInfo& info, std::vector<Segment>& headSegments) {
        for (auto& vec : info.blocks) {
            Segment segment;
            segment.pos = vec;
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
            glm::mat4 model = glm::translate(scale, vec);
            glm::mat4 move = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 25.0f, 0.0f));
            segment.push.model = move * model;
            segment.push.color = info.color;
            headSegments.push_back(segment);
        }
    }

    void createBody() {
        Segment head;
        head.pos = glm::vec3(0.0f, 35.0f, 0.0f);
        segments.push_back(head);
        for (int i = 0; i < 10; i++) {
            Segment segment;

            segment.pos = segments.back().pos + segmentDist;
            segments.push_back(segment);
        }
    }

    void createVertexBuffer() {
        BasiliskVertex positions[] = {
            // Front face
            {{-0.5f, -0.5f,  0.5f}}, {{ 0.5f, -0.5f,  0.5f}}, {{ 0.5f,  0.5f,  0.5f}},
            {{-0.5f, -0.5f,  0.5f}}, {{ 0.5f,  0.5f,  0.5f}}, {{-0.5f,  0.5f,  0.5f}},
            // Back face
            {{ 0.5f, -0.5f, -0.5f}}, {{-0.5f, -0.5f, -0.5f}}, {{-0.5f,  0.5f, -0.5f}},
            {{ 0.5f, -0.5f, -0.5f}}, {{-0.5f,  0.5f, -0.5f}}, {{ 0.5f,  0.5f, -0.5f}},
            // Left face
            {{-0.5f, -0.5f, -0.5f}}, {{-0.5f, -0.5f,  0.5f}}, {{-0.5f,  0.5f,  0.5f}},
            {{-0.5f, -0.5f, -0.5f}}, {{-0.5f,  0.5f,  0.5f}}, {{-0.5f,  0.5f, -0.5f}},
            // Right face
            {{ 0.5f, -0.5f,  0.5f}}, {{ 0.5f, -0.5f, -0.5f}}, {{ 0.5f,  0.5f, -0.5f}},
            {{ 0.5f, -0.5f,  0.5f}}, {{ 0.5f,  0.5f, -0.5f}}, {{ 0.5f,  0.5f,  0.5f}},
            // Top face
            {{-0.5f,  0.5f,  0.5f}}, {{ 0.5f,  0.5f,  0.5f}}, {{ 0.5f,  0.5f, -0.5f}},
            {{-0.5f,  0.5f,  0.5f}}, {{ 0.5f,  0.5f, -0.5f}}, {{-0.5f,  0.5f, -0.5f}},
            // Bottom face
            {{-0.5f, -0.5f, -0.5f}}, {{ 0.5f, -0.5f, -0.5f}}, {{ 0.5f, -0.5f,  0.5f}},
            {{-0.5f, -0.5f, -0.5f}}, {{ 0.5f, -0.5f,  0.5f}}, {{-0.5f, -0.5f,  0.5f}},
        };

        VkDeviceSize size = sizeof(positions);
        renderer->createBuffer(size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            buffer, memory);

        void* data;
        vkMapMemory(renderer->ctx->device, memory, 0, size, 0, &data);
        memcpy(data, positions, size);
        vkUnmapMemory(renderer->ctx->device, memory);
    }

    void cleanup() {
        vkDestroyBuffer(renderer->ctx->device, buffer, nullptr);
        vkFreeMemory(renderer->ctx->device, memory, nullptr);
    }
};