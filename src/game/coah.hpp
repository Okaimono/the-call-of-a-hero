#pragma once
#include "vulkan/renderer.hpp"
#include "vulkan/renderer_resource_manager.hpp"
#include "game/player/player.hpp"
#include "game/basilisk/basilisk.hpp"
#include "game/physics/physics_engine.hpp"

#include <functional>

class CallOfAHero {
public:
    World world;
    Player player;
    Basilisk basilisk;
    PhysicsEngine physicsEngine;

    Renderer* renderer;
    RendererResourceManager* rResourceManager;

    void init(Renderer* renderer, RendererResourceManager* rResourceManager) {
        this->renderer = renderer;
        this->rResourceManager = rResourceManager;
        world.init();
        basilisk.init(renderer, rResourceManager);
        createChunkSlots();
    }

    void update(InputManager* inputManager, float dt) {
        player.processInput(inputManager, dt);
        basilisk.update(dt, player.position);
        world.update(player.getPosition());
    }

    void tick(float dt) {
        //physicsEngine.update(dt, player.position, player.velocity, player.acceleration);

    }

    void render() {
        glm::mat4 view = player.getViewMatrix();
        glm::mat4 proj = player.getProjectionMatrix();
        renderer->updateUniformBuffer(view, proj);

        RenderQueue renderQueue{};
        submitCalls(renderQueue);

        renderer->drawFrame(world, renderQueue);

    }

    void submitCalls(RenderQueue& renderQueue) {
        size_t pushSize = sizeof(BasiliskPush);
        GfxPipeline& baskPipeline = rResourceManager->getPipeline(basilisk.gfxPipeline);

        for (auto& segment : basilisk.segments) {
            DrawCall call{};
            call.pipeline = baskPipeline.pipeline;
            call.layout = baskPipeline.layout;
            call.descriptorSet = renderer->descriptorSet;
            call.vertexBuffer = basilisk.buffer;
            call.pushConstant = true;
            call.pushSize = pushSize;
            call.pushData = &segment.push;
            call.vertexCount = 36;
            call.firstVertex = 0;
            renderQueue.push(call);
        }

        for (auto& segment : basilisk.head.headSegments) {
            DrawCall call{};
            call.pipeline = baskPipeline.pipeline;
            call.layout = baskPipeline.layout;
            call.descriptorSet = renderer->descriptorSet;
            call.vertexBuffer = basilisk.buffer;
            call.pushConstant = true;
            call.pushSize = pushSize;
            call.pushData = &segment.push;
            call.vertexCount = 36;
            call.firstVertex = 0;
            renderQueue.push(call);
        }

        // for (auto& chunk : world.renderedChunks) {
        //     auto it = world.worldGrid.find(chunk);
        //     if (it == world.worldGrid.end()) continue;

        //     Chunk& c = it->second;
        //     if (c.faces.empty()) continue;

        //     DrawCall call{};
        //     call.pipeline     = renderer->pipeline;
        //     call.layout       = renderer->pipelineLayout;
        //     call.descriptorSet = renderer->descriptorSet;
        //     call.vertexBuffer = VK_NULL_HANDLE;
        //     call.pushConstant = true;
        //     call.pushSize     = sizeof(glm::mat4);
        //     glm::mat4 chunkModel = glm::translate(glm::mat4(1.0f), glm::vec3(chunk.x * 16.0f, 0.0f, chunk.z * 16.0f));
        //     call.pushData     = nullptr;  // points into the struct itself
        //     call.vertexCount  = c.faces.size() * 6;
        //     call.firstVertex  = c.slot.slotOffset / sizeof(uint32_t);
        //     renderQueue.push(call);
        // }
    }

    void createChunkSlots() {
        std::unordered_map<ChunkCoord, Chunk, ChunkCoordHash>& worldGrid = world.worldGrid;
        for (auto& [key, value] : worldGrid) {
            value.slot = renderer->allocator.reserveSlot();
            
            const std::vector<uint32_t>& data = value.faces;
            renderer->allocator.updateSlot(value.slot, data);
        }
    }
};