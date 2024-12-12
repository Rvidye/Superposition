#version 460 core 
layout (binding = 0)uniform sampler2D texSampler; 
layout(location = 0)out vec4 FragColor;

layout(location = 0)in vec2 oTex;

void main(void) 
{
    vec4 color = texture(texSampler, oTex);
    FragColor = color;
};