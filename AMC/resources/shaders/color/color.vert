#version 460 core

layout(location = 0) in vec4 vPos;

layout(location = 0) uniform mat4 mvpMatrix; 

void main(void) {
    gl_Position = mvpMatrix * vPos;
}
