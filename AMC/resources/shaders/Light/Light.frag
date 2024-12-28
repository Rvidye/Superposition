#version 460 core

#extension GL_ARB_bindless_texture : require
#extension GL_EXT_shader_image_load_formatted : require

#include<..\..\..\resources\shaders\include\CommonTypes.glsl>
#include<..\..\..\resources\shaders\include\StaticUniformBuffers.glsl>
#include<..\..\..\resources\shaders\include\Compression.glsl>

layout(location = 0) out vec4 OutFragColor;
layout(location = 1) out vec2 OutNormal;
layout(location = 3) out vec3 OutEmissive;

in InOutData
{
    vec3 LightColor;
    vec3 FragPos;
    vec4 ClipPos;
    flat vec3 Position;
    flat float Radius;
} inData;

void main(void) {
    OutFragColor = vec4(inData.LightColor, 1.0);
    OutNormal = EncodeUnitVec((inData.FragPos - inData.Position) / inData.Radius);
    OutEmissive = OutFragColor.rgb;
}
