#pragma once
#include "../core/types.hpp"
#include "vulkan_includes.hpp"
#include "../vulkan/buffer.hpp"

class Chunk {
public:
    static constexpr int WIDTH  = 16;
    static constexpr int HEIGHT = 256;
    static constexpr int DEPTH  = 16;

    uint8_t blocks[WIDTH][HEIGHT][DEPTH];

    Buffer vertexBuffer;
    uint32_t vertexCount = 0;

    glm::ivec3 position{};
    bool dirty = true;

    Chunk() {
        memset(blocks, 0, sizeof(blocks));
        for (int x = 0; x < WIDTH; x++)
        for (int z = 0; z < DEPTH; z++)
        for (int y = 0; y < HEIGHT / 2; y++)
            blocks[x][y][z] = 0;
    }

    bool isAir(int x, int y, int z) {
        if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT || z < 0 || z >= DEPTH)
            return true;
        return blocks[x][y][z] == 0;
    }

    ChunkMesh createMesh(glm::ivec2 worldPos) {
        ChunkMesh mesh;

        for (int x = 0; x < WIDTH; x++)
        for (int y = 0; y < HEIGHT; y++)
        for (int z = 0; z < DEPTH; z++) {
            if (blocks[x][y][z] == 0) continue;

            float fx = x + worldPos.x * WIDTH;
            float fy = y;
            float fz = z + worldPos.y * DEPTH;

            // +Z (front)
            if (isAir(x, y, z+1)) {
                mesh.vertices.push_back({{fx+0,fy+0,fz+1},{1,1,1},{0,0}});
                mesh.vertices.push_back({{fx+1,fy+0,fz+1},{1,1,1},{1,0}});
                mesh.vertices.push_back({{fx+1,fy+1,fz+1},{1,1,1},{1,1}});
                mesh.vertices.push_back({{fx+0,fy+0,fz+1},{1,1,1},{0,0}});
                mesh.vertices.push_back({{fx+1,fy+1,fz+1},{1,1,1},{1,1}});
                mesh.vertices.push_back({{fx+0,fy+1,fz+1},{1,1,1},{0,1}});
            }
            // -Z (back)
            if (isAir(x, y, z-1)) {
                mesh.vertices.push_back({{fx+1,fy+0,fz+0},{1,1,1},{0,0}});
                mesh.vertices.push_back({{fx+0,fy+0,fz+0},{1,1,1},{1,0}});
                mesh.vertices.push_back({{fx+0,fy+1,fz+0},{1,1,1},{1,1}});
                mesh.vertices.push_back({{fx+1,fy+0,fz+0},{1,1,1},{0,0}});
                mesh.vertices.push_back({{fx+0,fy+1,fz+0},{1,1,1},{1,1}});
                mesh.vertices.push_back({{fx+1,fy+1,fz+0},{1,1,1},{0,1}});
            }
            // +X (right)
            if (isAir(x+1, y, z)) {
                mesh.vertices.push_back({{fx+1,fy+0,fz+1},{1,1,1},{0,0}});
                mesh.vertices.push_back({{fx+1,fy+0,fz+0},{1,1,1},{1,0}});
                mesh.vertices.push_back({{fx+1,fy+1,fz+0},{1,1,1},{1,1}});
                mesh.vertices.push_back({{fx+1,fy+0,fz+1},{1,1,1},{0,0}});
                mesh.vertices.push_back({{fx+1,fy+1,fz+0},{1,1,1},{1,1}});
                mesh.vertices.push_back({{fx+1,fy+1,fz+1},{1,1,1},{0,1}});
            }
            // -X (left)
            if (isAir(x-1, y, z)) {
                mesh.vertices.push_back({{fx+0,fy+0,fz+0},{1,1,1},{0,0}});
                mesh.vertices.push_back({{fx+0,fy+0,fz+1},{1,1,1},{1,0}});
                mesh.vertices.push_back({{fx+0,fy+1,fz+1},{1,1,1},{1,1}});
                mesh.vertices.push_back({{fx+0,fy+0,fz+0},{1,1,1},{0,0}});
                mesh.vertices.push_back({{fx+0,fy+1,fz+1},{1,1,1},{1,1}});
                mesh.vertices.push_back({{fx+0,fy+1,fz+0},{1,1,1},{0,1}});
            }
            // +Y (top)
            if (isAir(x, y+1, z)) {
                mesh.vertices.push_back({{fx+0,fy+1,fz+1},{1,1,1},{0,0}});
                mesh.vertices.push_back({{fx+1,fy+1,fz+1},{1,1,1},{1,0}});
                mesh.vertices.push_back({{fx+1,fy+1,fz+0},{1,1,1},{1,1}});
                mesh.vertices.push_back({{fx+0,fy+1,fz+1},{1,1,1},{0,0}});
                mesh.vertices.push_back({{fx+1,fy+1,fz+0},{1,1,1},{1,1}});
                mesh.vertices.push_back({{fx+0,fy+1,fz+0},{1,1,1},{0,1}});
            }
            // -Y (bottom)
            if (isAir(x, y-1, z)) {
                mesh.vertices.push_back({{fx+1,fy+0,fz+1},{1,1,1},{0,0}});
                mesh.vertices.push_back({{fx+0,fy+0,fz+1},{1,1,1},{1,0}});
                mesh.vertices.push_back({{fx+0,fy+0,fz+0},{1,1,1},{1,1}});
                mesh.vertices.push_back({{fx+1,fy+0,fz+1},{1,1,1},{0,0}});
                mesh.vertices.push_back({{fx+0,fy+0,fz+0},{1,1,1},{1,1}});
                mesh.vertices.push_back({{fx+1,fy+0,fz+0},{1,1,1},{0,1}});
            }
        }
        return mesh;
    }
};