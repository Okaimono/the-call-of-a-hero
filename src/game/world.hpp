#pragma once
#include "game/chunk.hpp"
#include <unordered_map>
#include <memory>

/*
THE EVIL ONE BUILDS HIS KINGDOM,

While YOU build YOUR kingdom

You must find the arrow remains through deep searching underground,
in the biome the Abyss,
an old ancient biome that was a warzone, now the home of the leviathan.

the final part before the boss,
is grief, desolation, despair, desolation

As you play, you learn both about the Leviathan,
and the Almighty
*/

struct PerlinChunk {
    ChunkCoord coord;
    float noise[16][16];
};

extern "C" void launchNoiseMap(int x, int z, PerlinChunk* chunks);

class World {
public:
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> worldGrid;
    std::unique_ptr<PerlinChunk[]> perlin;
    std::vector<ChunkCoord> renderedChunks;

    const int renderDist = 100;

    int worldLength = 30;
    int worldDepth = 30;

    void update(glm::vec3 playerPos) {
        getRenderedChunks(playerPos);
    }

    void init() {
        generatePerlin();
        //launchNoiseMap(-5, 5, -5, 5, (float*)noise);
        for (int x = -worldLength / 2; x < worldLength / 2; x++) {
            for (int z = -worldDepth / 2; z < worldDepth / 2; z++) {
                worldGrid[{x, z}] = std::make_unique<Chunk>(x, z, (float*)perlin[(x + worldLength / 2) * worldDepth + (z + worldDepth / 2)].noise);
            }
        }
    }

    void generatePerlin() {
        perlin = std::make_unique<PerlinChunk[]>(worldLength * worldDepth);
        for (int x = -worldLength / 2; x < worldLength / 2; x++) {
            for (int z = -worldDepth / 2; z < worldDepth / 2; z++) {
                perlin[(x + worldLength / 2) * worldDepth + (z + worldDepth / 2)].coord = {x, z};
            }
        }
        launchNoiseMap(worldLength, worldDepth, perlin.get());
    }

    void getRenderedChunks(glm::vec3 position) {
        renderedChunks.clear();
        for (int x = -renderDist; x <= renderDist; x++) {
            for (int z = -renderDist; z <= renderDist; z++) {
                if (x * x + z * z <= renderDist * renderDist) {

                    int playerChunkX = (int)floor(position.x / 16.0f);
                    int playerChunkZ = (int)floor(position.z / 16.0f);

                    ChunkCoord coords = {x + playerChunkX, z + playerChunkZ};
                    auto it = worldGrid.find(coords);
                    if (it != worldGrid.end()) {
                        renderedChunks.push_back(coords);
                    }
                }
            }
        }
    }

};

// each chunk has 16 x 16
// each block represents a chunk
// start 