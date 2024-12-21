#version 460 core

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