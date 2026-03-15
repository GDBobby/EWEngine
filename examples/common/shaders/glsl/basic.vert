#version 450

#include "GlobalPushConstant.glsl"

struct Vertex{
    vec2 pos;
    vec3 color;
};

layout(buffer_reference, scalar) readonly buffer Vertex_Buffer {
    Vertex vertices[];
};


#define ARBITRARY_MAX_BUFFER_COUNT 8
#define ARBITRARY_MAX_TEXTURE_COUNT 8
layout(push_constant) uniform Push {
    //0 is invalid
    Vertex_Buffer vertex_address;
    uint64_t buffer_address[7];
    // [-1] is the invalid index
    int texture_index[ARBITRARY_MAX_TEXTURE_COUNT];
} push;

layout(location = 0) out vec3 outColor;

void main(){

    Vertex_Buffer v_b = push.vertex_address;
    Vertex local_vert = v_b.vertices[gl_VertexIndex];
    gl_Position = vec4(local_vert.pos, 0.0, 1.0);
        
    outColor = local_vert.color;
}