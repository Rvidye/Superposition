#version 460 core

layout(location = 4) uniform vec3  u_LightPos;
layout(location = 5) uniform float u_FarPlane;

in ShadowData
{
    vec3 WorldPos;
} inData;

void main()
{
    float lightDistance = length(inData.WorldPos - u_LightPos);
    gl_FragDepth = lightDistance / u_FarPlane;
}
