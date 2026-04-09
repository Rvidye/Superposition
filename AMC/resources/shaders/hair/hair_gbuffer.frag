#version 460 core

#extension GL_ARB_bindless_texture : require

#include<..\..\..\resources\shaders\include\CommonTypes.glsl>
#include<..\..\..\resources\shaders\include\Compression.glsl>
#include<..\..\..\resources\shaders\include\HairTypes.glsl>

layout(location = 0) out vec4 OutAlbedoAlpha;
layout(location = 1) out vec2 OutNormal;
layout(location = 2) out vec2 OutMetallicRoughness;
layout(location = 3) out vec3 OutEmissive;
layout(location = 4) out vec2 OutVelocity;

in InOutData
{
    vec3 HairTangent;
    vec4 CenterClipPos;
    vec4 CenterPrevClipPos;
} inData;

void main()
{
    vec2 currentNDC = inData.CenterClipPos.xy / inData.CenterClipPos.w;
    vec2 prevNDC = inData.CenterPrevClipPos.xy / inData.CenterPrevClipPos.w;

    OutAlbedoAlpha = vec4(vec3(0.25, 0.15, 0.08), HAIR_SENTINEL);
    OutNormal = EncodeUnitVec(normalize(inData.HairTangent));
    OutMetallicRoughness = vec2(0.0, 0.6);
    OutEmissive = vec3(0.0);
    OutVelocity = (currentNDC - prevNDC) * 0.5;
}
