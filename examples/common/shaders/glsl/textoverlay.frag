#version 450 core

#extension GL_EXT_nonuniform_qualifier : enable

#include "GlobalPushConstant.glsl"

layout (location = 0) in vec2 inUV;

layout(set = 0, binding = 1) uniform sampler2D bindless[];

layout (location = 0) out vec4 outFragColor;

void main(void) {
	float color = texture(bindless[push.texture_index[0]], inUV).r;
	outFragColor = vec4(color);
}

//based on some old version of this https://github.com/SaschaWillems/Vulkan/blob/master/shaders/glsl/base/textoverlay.vert