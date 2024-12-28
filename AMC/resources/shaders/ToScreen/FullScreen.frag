#version 460 core

#extension GL_ARB_bindless_texture : require
#extension GL_EXT_shader_image_load_formatted : require

layout(location = 0) out vec4 OutFragColor;

in InOutData
{
    vec2 TexCoord;
} inData;

layout(location = 0) uniform float fade;
layout(binding = 0) uniform sampler2D textureResult;

void main()
{
    vec3 color = texture(textureResult,inData.TexCoord).rgb;
    vec3 result = mix(color, vec3(0.0), fade);
    OutFragColor = vec4(result,1.0);
}