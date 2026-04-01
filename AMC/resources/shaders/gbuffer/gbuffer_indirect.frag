#version 460 core

#extension GL_ARB_bindless_texture : require
#extension GL_EXT_shader_image_load_formatted : require

#include<..\..\..\resources\shaders\include\CommonTypes.glsl>
#include<..\..\..\resources\shaders\include\Transformations.glsl>
#include<..\..\..\resources\shaders\include\Compression.glsl>
#include<..\..\..\resources\shaders\include\Surface.glsl>

layout (location = 0)out vec4 OutAlbedoAlpha;
layout (location = 1)out vec2 OutNormal;
layout (location = 2)out vec2 OutMetallicRoughness;
layout (location = 3)out vec3 OutEmissive;
layout (location = 4)out vec2 OutVelocity;

// Only the material SSBO — no StaticUniformBuffers.glsl to avoid SSBO bloat
layout(std430, binding = 2) restrict readonly buffer MaterialSSBO
{
    GpuMaterial Materials[];
} materialSSBO;

in InOutData
{
    vec2 TexCoord;
    vec3 Normal;
    vec3 Tangent;
    vec4 ClipPos;
    vec4 PrevClipPos;
    flat uint MaterialId;
} inData;

void main(void)
{
    GpuMaterial material = materialSSBO.Materials[inData.MaterialId];
    Surface surface = GetSurface(material, inData.TexCoord);
    if (surface.Alpha < surface.AlphaCutoff)
    {
        discard;
    }
    // Compute TBN matrix for normal transformation
    vec3 interpTangent = normalize(inData.Tangent);
    vec3 interpNormal = normalize(inData.Normal);
    mat3 tbn = GetTBN(interpTangent,interpNormal);
    surface.Normal = tbn * surface.Normal;
    surface.Normal = normalize(mix(interpNormal, surface.Normal, material.TransmissionFactor));
    // Output to GBuffer
    OutAlbedoAlpha = vec4(surface.Albedo, surface.Alpha);
    OutNormal = EncodeUnitVec(surface.Normal);
    OutMetallicRoughness = vec2(surface.Metallic, surface.Roughness);
    OutEmissive = surface.Emissive;
    // Velocity: (currentNDC - prevNDC) * 0.5
    vec2 currentNDC = inData.ClipPos.xy / inData.ClipPos.w;
    vec2 prevNDC = inData.PrevClipPos.xy / inData.PrevClipPos.w;
    OutVelocity = (currentNDC - prevNDC) * 0.5;
}
