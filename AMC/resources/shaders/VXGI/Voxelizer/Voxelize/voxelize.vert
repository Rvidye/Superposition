#version 460 core

#extension GL_ARB_bindless_texture : require
#extension GL_EXT_shader_image_load_formatted : require

#include<..\..\..\resources\shaders\include\CommonTypes.glsl>
#include<..\..\..\resources\shaders\include\StaticUniformBuffers.glsl>
#include<..\..\..\resources\shaders\include\Transformations.glsl>

layout(location = 0)in vec3 vPos;
layout(location = 1)in vec3 vNor;
layout(location = 2)in vec2 vTex;

layout(location = 0) uniform mat4 modelMat;
layout(location = 1) uniform mat4 nodeMat;

out InOutData
{
    vec3 FragPos;
    vec2 TexCoord;
    vec3 Normal;
} outData;

void main(void) 
{
    outData.FragPos = (modelMat * nodeMat * vec4(vPos,1.0)).xyz;
    outData.TexCoord = vTex;
    mat3 normalMatrix = mat3(transpose(inverse(modelMat * nodeMat)));
    outData.Normal = normalize(normalMatrix * vNor);
    vec3 ndc = MapToZeroOne(outData.FragPos, voxelizerDataUBO.GridMin, voxelizerDataUBO.GridMax) * 2.0 - 1.0;
    gl_Position = vec4(ndc,1.0);
};