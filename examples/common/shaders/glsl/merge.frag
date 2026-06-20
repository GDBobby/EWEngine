#version 450

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "GlobalPushConstant.glsl"

layout(set = 0, binding = 1) uniform sampler2D bindless[];

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

//texture descriptor set

#define MAX_MERGE_COUNT 4

layout(push_constant) uniform Push {
    int texture_index[MAX_MERGE_COUNT];
} push;

void main(){

    vec4 total_color = vec4(0.0);
    int merge_count = 0;
    for(int i = 0; i < MAX_MERGE_COUNT; i++){
        if(push.texture_index[i] != null_texture){
            total_color += texture(bindless[push.texture_index[i]], fragUV);
            merge_count++;
        }
    }
    total_color /= merge_count;

    //debug print here maybe? if merge count is still 0?

    outColor = total_color; //clamp 0 to 1?
}