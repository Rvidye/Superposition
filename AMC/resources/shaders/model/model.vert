#version 460 core 
#extension GL_ARB_bindless_texture : require
#include<..\..\..\resources\shaders\include\StaticUniformBuffers.glsl>

layout(location = 0)in vec3 vPos;
layout(location = 1)in vec3 vNor;
layout(location = 2)in vec2 vTex;
layout(location = 3)in vec3 vTangent;
layout(location = 4)in vec3 vBitangent;

layout(location = 0) uniform mat4 mvpMat;
layout(location = 1) uniform mat4 nodeMat; 

layout(location = 0)out vec2 oTex;



void main(void) 
{
    gl_Position = perFrameDataUBO.ProjView * mvpMat * nodeMat * vec4(vPos,1.0); 
    oTex = vTex;
};