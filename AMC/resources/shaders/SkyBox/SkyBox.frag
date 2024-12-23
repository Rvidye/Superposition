#version 460 core

#extension GL_ARB_bindless_texture : require
#extension GL_EXT_shader_image_load_formatted : require

layout(location = 0) out vec4 OutFragColor;
in vec3 TexCoord;

#include<..\..\..\resources\shaders\include\CommonTypes.glsl>
#include<..\..\..\resources\shaders\include\StaticUniformBuffers.glsl>

void main()
{
    OutFragColor = vec4(texture(skyBoxUBO.Albedo,TexCoord).rgb,1.0);
}

