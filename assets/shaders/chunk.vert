layout(location = 0) in uint packedData; // same binding

void main() {
    float x = float(packedData & 0xF);
    float y = float((packedData >> 4) & 0x1F);
    float z = float((packedData >> 9) & 0xF);
    uint  face = (packedData >> 13) & 0x7;

    // local quad corners per face direction
    vec3 corners[4];
    if (face == 0) { // Top
        corners[0] = vec3(0,1,0);
        corners[1] = vec3(1,1,0);
        corners[2] = vec3(1,1,1);
        corners[3] = vec3(0,1,1);
    }
    // etc for other faces...

    uint vi = gl_VertexIndex % 6;
    uint idx = uint[6](0,1,2,2,3,0)[vi];
    vec3 pos = vec3(x,y,z) + corners[idx];
    gl_Position = ubo.proj * ubo.view * vec4(pos, 1.0);
}