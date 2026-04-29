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
    std::unordered_map<ChunkCoord, Chunk, ChunkCoordHash> worldGrid;
    std::unique_ptr<PerlinChunk[]> perlin;
    std::vector<ChunkCoord> renderedChunks;

    const int renderDist = 4;

    void init() {
        generatePerlin();
        //launchNoiseMap(-5, 5, -5, 5, (float*)noise);
        for (int x = -10; x < 10; x++) {
            for (int z = -10; z < 10; z++) {
                worldGrid[{x, z}] = Chunk(x, z, (float*)perlin[(x + 10) * 20 + (z + 10)].noise);
            }
        }
    }

    void generatePerlin() {
        perlin = std::make_unique<PerlinChunk[]>(20 * 20);
        for (int x = -10; x < 10; x++) {
            for (int z = -10; z < 10; z++) {
                perlin[(x + 10) * 20 + (z + 10)].coord = {x, z};
            }
        }
        launchNoiseMap(20, 20, perlin.get());
    }

};

// each chunk has 16 x 16
// each block represents a chunk
// start 