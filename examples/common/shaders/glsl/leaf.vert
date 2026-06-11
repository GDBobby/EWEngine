#version 450

#include "leaf_common.glsl"

layout(location = 0) out vec3 fragPosWorld;
layout(location = 1) out vec3 fragNormalWorld;
//layout(location = 2) out vec2 fragTexCoord;

void main(){

	vec4 positionWorld = push.ir.leafMatrices[gl_InstanceIndex] * (vec4(push.vertices.vertices[gl_VertexIndex].position, 1.0));
	gl_Position = push.ir.sbo.projView * positionWorld;
	
	fragPosWorld = positionWorld.xyz;
	fragNormalWorld = push.vertices.vertices[gl_VertexIndex].normal;
	//fragTexCoord = push.vertices.vertices[gl_VertexIndex].uv;
}