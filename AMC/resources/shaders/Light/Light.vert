#version 460 core

#extension GL_ARB_bindless_texture : require
#extension GL_EXT_shader_image_load_formatted : require

#include<..\..\..\resources\shaders\include\CommonTypes.glsl>
#include<..\..\..\resources\shaders\include\StaticUniformBuffers.glsl>

layout(location = 0) in vec4 vPos;

out InOutData
{
    vec3 LightColor;
    vec3 FragPos;
    vec4 ClipPos;
    vec3 Position;
    float Radius;
} outData;

layout(location = 0)uniform int lightIndex;

void main(void) {

    Light light = u_Lights[lightIndex];

    mat4x3 modelMatrix = mat4x3(
    vec3(light.range, 0.0, 0.0),
    vec3(0.0, light.range, 0.0),
    vec3(0.0, 0.0, light.range),
    vec3(light.position)
    );

    outData.LightColor = light.color;
    outData.FragPos = modelMatrix * vec4(vPos.xyz, 1.0);
    outData.ClipPos = perFrameDataUBO.ProjView * vec4(outData.FragPos, 1.0);
    outData.Position = light.position;
    outData.Radius = light.range;

    // Add jitter independent of perspective by multiplying with w
    vec4 jitteredClipPos = outData.ClipPos;
    jitteredClipPos.xy += vec2(0.0,0.0) * outData.ClipPos.w;

    gl_Position = jitteredClipPos;
}
