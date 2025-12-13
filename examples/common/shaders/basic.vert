#version 450

#include "GlobalPushConstant.glsl"

layout(buffer_reference, scalar) readonly buffer Vertex {
    vec2 position;
    vec3 color;
};

layout(location = 0) out vec3 outColor;

void main(){

    Vertex vertex = Vertex(push.device_addresses[0]) + gl_VertexIndex;
    gl_Position = vec4(vertex.position, 0.0, 1.0);
  
    outColor = vertex.color;
}