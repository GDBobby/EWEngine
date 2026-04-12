#version 450 core

#include "GlobalPushConstant.glsl"

layout (location = 0) out vec2 outUV;

layout(buffer_reference, std430) buffer CharData_Buffer {
    vec4 posUV[];
};

layout(push_constant) uniform Push {
    uint64_t buffer_address[ARBITRARY_MAX_BUFFER_COUNT];
    int texture_index[ARBITRARY_MAX_TEXTURE_COUNT];
} push;

void main() {
	CharData_Buffer temp_ptr = CharData_Buffer(push.buffer_address[0]);
	vec4 local_posUV = temp_ptr.posUV[gl_VertexIndex + gl_InstanceIndex * 4];
	
	gl_Position = vec4(local_posUV.xy, 0.0, 1.0);
	outUV = local_posUV.zw;
}

//based on some old version of this https://github.com/SaschaWillems/Vulkan/blob/master/shaders/glsl/base/textoverlay.vert