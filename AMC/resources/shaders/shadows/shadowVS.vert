#version 460 core 

layout(location = 0)in vec3 vPos;

layout(location = 0) uniform mat4 model;
layout(location = 1) uniform mat4 nodeMat;

void main(void) 
{
    gl_Position = model * nodeMat * vec4(vPos,1.0);
};