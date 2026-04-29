#pragma once
#include "vulkan/renderer.hpp"
#include "game/player/player.hpp"

class CallOfAHero {
public:
    World world;
    Player player;

    Renderer* renderer;

    void init(Renderer* renderer) {
        this->renderer = renderer;
        world.init();
        createChunkSlots();
    }

    void update(InputManager* inputManager, float dt) {
        player.processInput(inputManager, dt);
    }

    void render() {
        glm::mat4 view = player.getViewMatrix();
        glm::mat4 proj = player.getProjectionMatrix();
        renderer->updateUniformBuffer(view, proj);

        renderer->drawFrame(world);
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