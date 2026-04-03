#version 460 core

#extension GL_ARB_bindless_texture : require
#extension GL_EXT_shader_image_load_formatted : require

// Shadow fragment shader for mesh-pipeline path.
// Computes linear depth from light position, same as legacy shadowFS.frag.

layout(location = 4) uniform vec3  u_LightPos;
layout(location = 5) uniform float u_FarPlane;

in ShadowData {
    vec3 WorldPos;
} inData;

void main()
{
    float lightDistance = length(inData.WorldPos - u_LightPos);
    gl_FragDepth = lightDistance / u_FarPlane;
}
