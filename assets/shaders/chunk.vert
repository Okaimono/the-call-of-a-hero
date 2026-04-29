#version 450

const int texID[4][6] = int[4][6](
    int[6](0, 0, 0, 0, 0, 0), // empty
    int[6](0, 0, 0, 0, 0, 0), // cobblestone
    int[6](1, 1, 1, 1, 1, 1), // dirt
    int[6](3, 1, 2, 2, 2, 2)  // grass block
);

const float TILE = 16.0 / 512.0;

const vec2 uvOrigin[4] = vec2[4] (
    vec2(TILE * 0, 0.0), // cobblestone
    vec2(TILE * 1, 0.0), // dirt
    vec2(TILE * 2, 0.0),  // grass side
    vec2(TILE * 3, 0.0)  // grass top
);

const vec2 uvCorners[4] = vec2[4](
    vec2(0.0, 0.0),
    vec2(TILE, 0.0),
    vec2(TILE, TILE),
    vec2(0.0, TILE)
);

// LAYOUT
layout(set = 0, binding = 0) uniform UBO {
    mat4 view;
    mat4 proj;
} ubo;

layout(set = 0, binding = 1) readonly buffer FaceBuffer {
    uint faces[];
};

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

layout(location = 0) out vec2 fragUV;

void main() {
    uint faceIdx = gl_InstanceIndex + gl_VertexIndex / 6;
    uint vi = gl_VertexIndex % 6;

    uint packed = faces[faceIdx];

    float x = float(packed & 0xF);
    float y = float((packed >> 4) & 0x1F);
    float z = float((packed >> 9) & 0xF);

    uint face = (packed >> 13) & 0x7;
    uint block = (packed >> 16) & 0xF; 

    vec3 corners[4];
    if (face == 0) {
        corners[0] = vec3(0,1,1); corners[1] = vec3(1,1,1);
        corners[2] = vec3(1,1,0); corners[3] = vec3(0,1,0);
    } else if (face == 1) {
        corners[0] = vec3(0,0,0); corners[1] = vec3(1,0,0);
        corners[2] = vec3(1,0,1); corners[3] = vec3(0,0,1);
    } else if (face == 2) {
        corners[0] = vec3(0,1,0); corners[1] = vec3(1,1,0);
        corners[2] = vec3(1,0,0); corners[3] = vec3(0,0,0);
    } else if (face == 3) {
        corners[0] = vec3(1,1,1); corners[1] = vec3(0,1,1);
        corners[2] = vec3(0,0,1); corners[3] = vec3(1,0,1);
    } else if (face == 4) {
        corners[0] = vec3(1,1,0); corners[1] = vec3(1,1,1);
        corners[2] = vec3(1,0,1); corners[3] = vec3(1,0,0);
    } else {
        corners[0] = vec3(0,1,1); corners[1] = vec3(0,1,0);
        corners[2] = vec3(0,0,0); corners[3] = vec3(0,0,1);
    }

    uint indices[6] = uint[6](0,1,2,2,3,0);

    int id = texID[block][face];

    fragUV = uvOrigin[id] + uvCorners[indices[vi]];

    vec3 pos = vec3(x,y,z) + corners[indices[vi]];
    gl_Position = ubo.proj * ubo.view * push.model * vec4(pos, 1.0);
}