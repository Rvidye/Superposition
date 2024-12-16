#version 460 core 

layout(location = 0)in vec3 vPos;
layout(location = 1)in vec2 vTex;
layout(location = 0) uniform mat4 mvpMat; 

layout(location = 0) out vec2 oTex;

void main(void) 
{  
    gl_Position = mvpMat * vec4(vPos,1.0); 
    oTex = vTex;
};