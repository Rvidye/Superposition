#version 460 core

layout(location = 0) out vec4 OutFragColor;
layout(binding = 0) uniform samplerCube Albedo;
in vec3 TexCoord;

void main()
{
    OutFragColor = vec4(texture(Albedo,TexCoord).rgb,1.0);//textureLod(Albedo, TexCoord, 0.0);
}

