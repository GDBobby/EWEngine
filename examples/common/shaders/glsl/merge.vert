#version 450

#include "GlobalPushConstant.glsl"

layout(location = 0) out vec2 outUV;

const vec2 fullscreen_positions[4] = vec2[4](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0,  1.0)
);

const vec2 fullscreen_uv[4] = vec2[4](
    vec2(0.0, 1.0),
    vec2(1.0, 1.0),
    vec2(0.0, 0.0),
    vec2(1.0, 0.0)
);

void main(){

    vec2 position = fullscreen_positions[gl_VertexIndex];
    gl_Position = vec4(position, 0.0, 1.0);

    outUV = fullscreen_uv[gl_VertexIndex];
}