#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require

//this needs to match the value in EightWinds/GlobalPushConstant.h
#define ARBITRARY_MAX_BUFFER_COUNT 8
#define ARBITRARY_MAX_TEXTURE_COUNT 8
layout(push_constant) uniform Push {
    //0 is invalid
    uint64_t device_addresses[ARBITRARY_MAX_BUFFER_COUNT];
    // [-1] is the invalid index
    int texture_index[ARBITRARY_MAX_TEXTURE_COUNT];
} push;