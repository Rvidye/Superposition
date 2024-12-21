#version 460 core

in vec4 FragPos;

layout(location = 2)uniform float far_plane;
layout(location = 3)uniform vec3 lightPos;

void main()
{
    float lightDistance = length(FragPos.xyz - lightPos);
    lightDistance = lightDistance / far_plane;
    gl_FragDepth = lightDistance;
}