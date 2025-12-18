#version 450

#extension GL_EXT_debug_printf : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "GlobalPushConstant.glsl"

layout(set = 0, binding = 1) uniform sampler2D bindless[];

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

//texture descriptor set

void main(){

    vec4 first_color = texture(bindless[push.texture_index[0]], fragUV);
    vec4 second_color = texture(bindless[push.texture_index[1]], fragUV);

    outColor = vec4(mix(first_color.rgb, second_color.rgb, second_color.a), 
                    first_color.a + second_color.a * (1.0 - first_color.a));
}