#version 460 core

#extension GL_ARB_bindless_texture : require
#extension GL_EXT_shader_image_load_formatted : require

#include<..\..\..\resources\shaders\include\CommonTypes.glsl>
#include<..\..\..\resources\shaders\include\StaticUniformBuffers.glsl>
#include<..\..\..\resources\shaders\include\Compression.glsl>

layout(location = 0)in vec3 vPos;
layout(location = 1)in uint vNor;
layout(location = 2)in vec2 vTex;
layout(location = 3)in uint vTangent;

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
    vec3 normal = DecompressSR11G11B10(vNor);
    vec3 tangent = DecompressSR11G11B10(vTangent);
    mat3 unitVecToWorld = mat3(transpose(inverse(modelMat * nodeMat)));
    outData.Normal = normalize(unitVecToWorld * normal);
    outData.Tangent = normalize(unitVecToWorld * tangent);
};