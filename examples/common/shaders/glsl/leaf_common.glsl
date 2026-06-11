#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : enable

struct Vertex{
	vec3 position;
	vec3 normal;
	vec2 uv;
};
struct PointLight{
	vec4 position; //ignore w
	vec4 color; //w is intensity
};

struct SceneBufferObject{
	mat4 projView;
	vec4 cameraPos;

	vec4 ambientColor;
	vec4 sunlightDirection; //w for sun power
	vec4 sunlightColor;
	//PointLight pointLights[10]; //max lights in frameinfo header
	//int numLights;
};

layout(buffer_reference, scalar) readonly buffer VertRef{
	Vertex vertices[];
};
layout(buffer_reference, scalar) readonly buffer TransRef{
	SceneBufferObject sbo;
	mat4 leafMatrices[1024];
};


layout(push_constant) uniform Push{
	VertRef vertices;
    TransRef ir;
	//int texture_index;
} push;