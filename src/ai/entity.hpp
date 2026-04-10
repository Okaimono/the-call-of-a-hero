#pragma once
#include "../core/types.hpp"
#include "../vulkan_includes.hpp"
#include "../vulkan/buffer.hpp"

enum class Direction {NORTH, EAST, SOUTH, WEST};

class Entity {
public:
    glm::vec3 position{0.0f, 17.0f, 0.0f}; // sits on top of the 16 block floor
    float stepSize = 0.1f;

    Buffer vertexBuffer;
    uint32_t vertexCount = 0;

    void walk(Direction dir) {
        switch (dir) {
            case Direction::NORTH: position.z += stepSize; break;
            case Direction::SOUTH: position.z -= stepSize; break;
            case Direction::EAST:  position.x += stepSize; break;
            case Direction::WEST:  position.x -= stepSize; break;
        }
    }

    ChunkMesh createMesh() {
        ChunkMesh mesh;
        float s = 0.5f; // half size — 50% of a block
        float o = 0.25f; // offset to center it in the block
        float x = position.x + o;
        float y = position.y;
        float z = position.z + o;

        // front
        mesh.vertices.push_back({{x+0,y+0,z+s},{0,1,0},{0,0}});
        mesh.vertices.push_back({{x+s,y+0,z+s},{0,1,0},{1,0}});
        mesh.vertices.push_back({{x+s,y+s,z+s},{0,1,0},{1,1}});
        mesh.vertices.push_back({{x+0,y+0,z+s},{0,1,0},{0,0}});
        mesh.vertices.push_back({{x+s,y+s,z+s},{0,1,0},{1,1}});
        mesh.vertices.push_back({{x+0,y+s,z+s},{0,1,0},{0,1}});
        // back
        mesh.vertices.push_back({{x+s,y+0,z+0},{0,1,0},{0,0}});
        mesh.vertices.push_back({{x+0,y+0,z+0},{0,1,0},{1,0}});
        mesh.vertices.push_back({{x+0,y+s,z+0},{0,1,0},{1,1}});
        mesh.vertices.push_back({{x+s,y+0,z+0},{0,1,0},{0,0}});
        mesh.vertices.push_back({{x+0,y+s,z+0},{0,1,0},{1,1}});
        mesh.vertices.push_back({{x+s,y+s,z+0},{0,1,0},{0,1}});
        // right
        mesh.vertices.push_back({{x+s,y+0,z+0},{0,1,0},{0,0}});
        mesh.vertices.push_back({{x+s,y+0,z+s},{0,1,0},{1,0}});
        mesh.vertices.push_back({{x+s,y+s,z+s},{0,1,0},{1,1}});
        mesh.vertices.push_back({{x+s,y+0,z+0},{0,1,0},{0,0}});
        mesh.vertices.push_back({{x+s,y+s,z+s},{0,1,0},{1,1}});
        mesh.vertices.push_back({{x+s,y+s,z+0},{0,1,0},{0,1}});
        // left
        mesh.vertices.push_back({{x+0,y+0,z+s},{0,1,0},{0,0}});
        mesh.vertices.push_back({{x+0,y+0,z+0},{0,1,0},{1,0}});
        mesh.vertices.push_back({{x+0,y+s,z+0},{0,1,0},{1,1}});
        mesh.vertices.push_back({{x+0,y+0,z+s},{0,1,0},{0,0}});
        mesh.vertices.push_back({{x+0,y+s,z+0},{0,1,0},{1,1}});
        mesh.vertices.push_back({{x+0,y+s,z+s},{0,1,0},{0,1}});
        // top
        mesh.vertices.push_back({{x+0,y+s,z+0},{0,1,0},{0,0}});
        mesh.vertices.push_back({{x+0,y+s,z+s},{0,1,0},{1,0}});
        mesh.vertices.push_back({{x+s,y+s,z+s},{0,1,0},{1,1}});
        mesh.vertices.push_back({{x+0,y+s,z+0},{0,1,0},{0,0}});
        mesh.vertices.push_back({{x+s,y+s,z+s},{0,1,0},{1,1}});
        mesh.vertices.push_back({{x+s,y+s,z+0},{0,1,0},{0,1}});
        // bottom
        mesh.vertices.push_back({{x+0,y+0,z+s},{0,1,0},{0,0}});
        mesh.vertices.push_back({{x+0,y+0,z+0},{0,1,0},{1,0}});
        mesh.vertices.push_back({{x+s,y+0,z+0},{0,1,0},{1,1}});
        mesh.vertices.push_back({{x+0,y+0,z+s},{0,1,0},{0,0}});
        mesh.vertices.push_back({{x+s,y+0,z+0},{0,1,0},{1,1}});
        mesh.vertices.push_back({{x+s,y+0,z+s},{0,1,0},{0,1}});

        return mesh;
    }
};