#version 460 core 

#include<..\..\..\resources\shaders\include\lighting.glsl>

layout (binding = 0)uniform sampler2D texSampler; 
layout(location = 0)out vec4 FragColor;
in vec2 oTex;
void main(void) 
{
    vec4 color = texture(texSampler, oTex);
    FragColor = color;
};