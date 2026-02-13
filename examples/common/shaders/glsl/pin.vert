#version 450

#include "GlobalPushConstant.glsl"

layout(location = 0) flat out int outIndex;
layout(location = 1) out float verticalPosition;
layout(location = 2) out float horizontalPosition;
layout(location = 3) out vec3 vert_color;

const vec2 defaultPositions[4] = vec2[4](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0,  1.0)
);

struct VertexData{
    vec3 color;

    vec2 position;
    vec2 scale;
};


#define MAXIMUM_NODES 1024
layout(buffer_reference, scalar) readonly buffer VertexArray {
    VertexData vertices[MAXIMUM_NODES];
};

void main(){

    VertexArray vertexArray = VertexArray(push.buffer_address[0]);
    VertexData vertData = vertexArray.vertices[gl_InstanceIndex];

    vec2 position = defaultPositions[gl_VertexIndex];
    verticalPosition = position.y;
    horizontalPosition = position.x;


    position = (position * vertData.scale) + vertData.position;
    gl_Position = vec4(position, 0.0, 1.0);
	
	vert_color = vertData.color;
  
    outIndex = gl_InstanceIndex;
}