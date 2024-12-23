#version 460 core

#extension GL_ARB_bindless_texture : require
#extension GL_EXT_shader_image_load_formatted : require

in InOutData
{
    vec2 TexCoord;
} inData;

layout(binding = 10) uniform sampler2D textureResult;
layout(location = 0) out vec4 OutFragColor;

void main()
{
    vec3 color = texture(textureResult,inData.TexCoord).rgb;
    OutFragColor = vec4(color,1.0);
}