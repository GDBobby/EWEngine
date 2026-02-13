#version 450

#extension GL_EXT_debug_printf : enable

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main(){
    debugPrintfEXT("drawing - (%f)(%f)(%f)", fragColor.r, fragColor.g, fragColor.b);
    outColor = vec4(fragColor, 1.0);
}