#version 460 core 

#extension GL_ARB_bindless_texture : require
#extension GL_EXT_shader_image_load_formatted : require

layout(location = 0)in vec3 vPos;
layout(location = 0) uniform mat4 model;
layout(location = 1) uniform mat4 nodeMat;

void main(void) 
{
    gl_Position = model * nodeMat * vec4(vPos,1.0);
};