#version 450

#include "GlobalPushConstant.glsl"

layout(buffer_reference, scalar) readonly buffer Vertex {
    vec2 position;
    vec3 color;
};

//layout(location = 0) out vec3 outColor;
layout(location = 0) out mat4 fullData;
layout(location = 4) flat out uint64_t address;

void main(){

    address = uint64_t(Vertex(push.device_addresses[0]) + gl_VertexIndex);

    Vertex vertexBuffer = Vertex(push.device_addresses[0]);
    Vertex vertex = vertexBuffer[0];
    gl_Position = vec4(vertex.position, 0.0, 1.0);

    fullData[0][0] = vertex.position.x;
    fullData[0][1] = vertex.position.y;
    fullData[0][2] = vertex.color.r;
    fullData[0][3] = vertex.color.g;

    fullData[1][0] = vertex.color.b;
    Vertex second_vertexBuffer = Vertex(push.device_addresses[0]) + 1;
    fullData[1][1] = second_vertexBuffer[0].position.x;
    fullData[1][2] = second_vertexBuffer[0].position.y;
    fullData[1][3] = second_vertexBuffer[0].color.r;

    fullData[2][0] = second_vertexBuffer[0].color.g;
    fullData[2][1] = second_vertexBuffer[0].color.b;
    
    Vertex third_vertexBuffer = Vertex(push.device_addresses[0]) + 2;
    fullData[2][2] = third_vertexBuffer[0].position.x;
    fullData[2][3] = third_vertexBuffer[0].position.y;

    fullData[3][0] = third_vertexBuffer[0].color.r;
    fullData[3][1] = third_vertexBuffer[0].color.g;
    fullData[3][2] = third_vertexBuffer[0].color.b;
  
    //outColor = vertex.color;
}