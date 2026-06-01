#pragma once
#include "game/world.hpp"

class BlockInteraction {
public:
    World* world = nullptr;

    void init(World* world) {
        this->world = world;
    }

    void breakBlock(const glm::vec3& position, const glm::vec3& orientation, float range) {
        auto& worldGrid = world->worldGrid;
        
        glm::vec3 ray = orientation;
        for (float i = 0; i < range * 5; i+= 0.1) {
            ray = orientation * i;
        }
        
    }
};