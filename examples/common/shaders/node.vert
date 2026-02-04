#version 450

#include "GlobalPushConstant.glsl"

layout(location = 0) flat out int outIndex;
layout(location = 1) out float verticalPosition;
layout(location = 2) out float horizontalPosition;

const vec2 defaultPositions[4] = vec2[4](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0,  1.0)
);

struct VertexData{
    vec3 titleColor;
    float titleScale;

    vec3 foregroundColor;
    float foregroundScale;

    vec3 backgroundColor;
    int objectType;

    vec4 position;
    vec2 scale;
};


#define MAXIMUM_NODES 1024
layout(buffer_reference, scalar) readonly buffer VertexArray {
    VertexData vertices[MAXIMUM_NODES];
};

void main(){

    VertexArray vertexArray = VertexArray(push.device_addresses[0]);
    VertexData vertData = vertexArray.vertices[gl_InstanceIndex];

    vec2 position = defaultPositions[gl_VertexIndex];
    verticalPosition = position.y;
    horizontalPosition = position.x;


    position = (position * vertData.scale) + vertData.position.xy;
    gl_Position = vec4(position, vertData.position.z, 1.0);
  
    outIndex = gl_InstanceIndex;
}