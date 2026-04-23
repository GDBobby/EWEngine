#version 450

#include "GlobalPushConstant.glsl"

struct Vertex{
    vec2 pos;
    vec3 color;
};

layout(buffer_reference, scalar) readonly buffer Vertex_Buffer {
    Vertex vertices[];
};


layout(push_constant) uniform Push {
    Vertex_Buffer vertex_address;
} push;

layout(location = 0) out vec3 outColor;

void main(){

    Vertex_Buffer v_b = push.vertex_address;
    Vertex local_vert = v_b.vertices[gl_VertexIndex];
    gl_Position = vec4(local_vert.pos, 0.0, 1.0);
        
    outColor = local_vert.color;
}