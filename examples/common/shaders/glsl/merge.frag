#version 450

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "GlobalPushConstant.glsl"

layout(set = 0, binding = 1) uniform sampler2D bindless[];

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

//texture descriptor set

layout(push_constant) uniform Push {
    //0 is invalid
    uint64_t buffer_address[ARBITRARY_MAX_BUFFER_COUNT];
    // [-1] is the invalid index
    int texture_index[ARBITRARY_MAX_TEXTURE_COUNT];
} push;

void main(){

    vec4 first_color = texture(bindless[push.texture_index[0]], fragUV);
    if(push.texture_index[1] < 0){
        outColor = first_color.rgba;
    }
    else{
        vec4 second_color = texture(bindless[push.texture_index[1]], fragUV);

        outColor = vec4(mix(first_color.rgb, second_color.rgb, second_color.a), 
                        first_color.a + second_color.a * (1.0 - first_color.a));
    }
}