#version 460 core

#extension GL_ARB_bindless_texture : require
#extension GL_EXT_shader_image_load_formatted : require

#include<..\..\..\resources\shaders\include\CommonTypes.glsl>
#include<..\..\..\resources\shaders\include\StaticUniformBuffers.glsl>

in vec4 FragPos;
layout(location = 4)uniform int shadowIndex;

void main()
{
    float lightDistance = length(FragPos.xyz - shadows[shadowIndex].Position);
    lightDistance = lightDistance / shadows[shadowIndex].FarPlane;
    gl_FragDepth = lightDistance;
}