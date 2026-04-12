#version 450 core

#extension GL_EXT_nonuniform_qualifier : enable

#include "GlobalPushConstant.glsl"

layout (location = 0) in vec2 inUV;

layout(set = 0, binding = 1) uniform sampler2D bindless[];

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform Push {
    //0 is invalid
    uint64_t buffer_address[ARBITRARY_MAX_BUFFER_COUNT];
    // [-1] is the invalid index
    int texture_index[ARBITRARY_MAX_TEXTURE_COUNT];
} push;

void main(void) {
	float color = texture(bindless[push.texture_index[0]], inUV).r;
	outFragColor = vec4(color);
}

//based on some old version of this https://github.com/SaschaWillems/Vulkan/blob/master/shaders/glsl/base/textoverlay.vert