#version 460 core

#include<..\..\..\resources\shaders\include\CommonTypes.glsl>
#include<..\..\..\resources\shaders\include\StaticUniformBuffers.glsl>

layout(location = 0)in vec3 vPos;
layout(location = 1)in vec3 vNor;
layout(location = 2)in vec2 vTex;
layout(location = 3)in vec3 vTangent;
layout(location = 4)in vec3 vBitangent;

layout(location = 0) uniform mat4 modelMat;
layout(location = 1) uniform mat4 nodeMat;

out InOutData
{
    vec2 TexCoord;
    vec3 Normal;
    vec3 Tangent;
} outData;

void main(void) 
{

    gl_Position = perFrameDataUBO.ProjView * modelMat * nodeMat * vec4(vPos,1.0);
    outData.TexCoord = vTex;
    mat3 normalMatrix = mat3(transpose(inverse(modelMat * nodeMat)));
    outData.Normal = normalize(normalMatrix * vNor);
    outData.Tangent = normalize(normalMatrix * vTangent);
};