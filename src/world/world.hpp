#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include "vulkan_includes.hpp"
#include "chunk.hpp"
#include "../ai/entity.hpp"

#include <glm/gtx/hash.hpp>
#include <unordered_map>

class World {
public:
    std::unordered_map<glm::ivec2, Chunk, std::hash<glm::ivec2>> chunks;
    Entity entity;

    void generateChunk(int cx, int cz) {
        glm::ivec2 coord{cx, cz};
        Chunk chunk;
        chunk.position = {cx, 0, cz};

        for (int x = 0; x < Chunk::WIDTH; x++)
        for (int z = 0; z < Chunk::DEPTH; z++)
        for (int y = 0; y < 16; y++)
            chunk.blocks[x][y][z] = 1; // cobblestone floor

        chunks[coord] = std::move(chunk);
    }

    void generateWorld(int radius) {
        for (int x = -radius; x <= radius; x++)
        for (int z = -radius; z <= radius; z++)
            generateChunk(x, z);
    }
};