#version 450

#include "GlobalPushConstant.glsl"

struct VertexData{
    
    vec3 titleColor;
    float titleScale;

    vec3 foregroundColor;
    float foregroundScale;

    vec3 backgroundColor;
    int objectType;

    vec4 position;
    vec2 scale;
};

#define MAXIMUM_NODES 1024

layout(buffer_reference, scalar) readonly buffer VertexArray {
    VertexData vertices[MAXIMUM_NODES];
};

layout(location = 0) flat in int outIndex;
layout(location = 1) in float verticalPosition;
layout(location = 2) in float horizontalPosition;

layout(location = 0) out vec4 fragColor;

void main(){

    VertexArray vertexArray = VertexArray(push.device_addresses[0]);
    VertexData vertData = vertexArray.vertices[outIndex];

    if(vertData.objectType == 0){
        //bool withinTitle = ((verticalPosition + 1.0) / 2.0) < vertData.titleScale;
        const bool withinTitle = verticalPosition < ((vertData.titleScale - 0.5) * 2.0);

        float absVertPos = abs(verticalPosition);
        float absHoriPos = abs(horizontalPosition);

        const bool withinBorder = (absVertPos < vertData.foregroundScale) && (absHoriPos < vertData.foregroundScale);

        if(withinTitle){
            fragColor = vec4(vertData.titleColor, 1.0);
        } 
        else if(withinBorder){
            fragColor = vec4(vertData.foregroundColor, 1.0);
        }
        else {
            fragColor = vec4(vertData.backgroundColor, 1.0);
        }
    }
    else if(vertData.objectType == 1){
        vec2 uv;
        uv.x = horizontalPosition;
        uv.y = verticalPosition;

        float sqrd_len = 3. * (uv.x * uv.x + uv.y * uv.y);
        
        float withinCircle = float(sqrd_len < 2.8);
        
        float colorVal = withinCircle * abs(1.0 - (
                    pow(sqrd_len, 3.0) - 3.0 * pow(sqrd_len, 2.0) 
                    + 4.0 + pow(sqrd_len, 2.0 / 16.0)
                ) / 4.0);
        
        fragColor = vec4(vertData.foregroundColor * colorVal, withinCircle);
    }
    else{
        fragColor = vec4(1.0);
    }
}