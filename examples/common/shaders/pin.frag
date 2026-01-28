#version 450

#include "GlobalPushConstant.glsl"

struct VertexData{
    vec3 color;

    vec2 position;
    vec2 scale;
};

#define MAXIMUM_NODES 1024

layout(buffer_reference, scalar) readonly buffer VertexArray {
    VertexData vertices[MAXIMUM_NODES];
};

layout(location = 0) flat in int outIndex;
layout(location = 1) in float verticalPosition;
layout(location = 2) in float horizontalPosition;
layout(location = 3) in vec3 vert_color;

layout(location = 0) out vec4 fragColor;

void main(){
	


}