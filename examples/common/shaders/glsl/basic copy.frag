#version 450

#include "GlobalPushConstant.glsl"

//layout(location = 0) in vec3 fragColor;
layout(location = 0) in mat4 fullData;
layout(location = 4) flat in uint64_t address;

layout(location = 0) out vec4 outColor;

void main(){
    outColor = vec4(fullData[0].rgb, 1.0);
}