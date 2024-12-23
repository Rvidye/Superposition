#version 460 core

#extension GL_ARB_bindless_texture : require
#extension GL_EXT_shader_image_load_formatted : require

#include<..\..\..\resources\shaders\include\CommonTypes.glsl>
#include<..\..\..\resources\shaders\include\Transformations.glsl>
#include<..\..\..\resources\shaders\include\StaticUniformBuffers.glsl>
#include<..\..\..\resources\shaders\include\Surface.glsl>

layout (location = 0)out vec4 OutAlbedoAlpha;
layout (location = 1)out vec3 OutNormal;
layout (location = 2)out vec2 OutMetallicRoughness;
layout (location = 3)out vec3 OutEmissive;

layout(location = 3)uniform Material material;

layout (binding = 0)uniform sampler2D BaseColorMap;
layout (binding = 1)uniform sampler2D NormalMap;
layout (binding = 2)uniform sampler2D MetallicRoughnessMap;
layout (binding = 3)uniform sampler2D EmissiveMap;


in InOutData
{
    vec2 TexCoord;
    vec3 Normal;
    vec3 Tangent;
} inData;

void main(void) 
{

    vec4 albedo = vec4(material.albedo, material.alpha);
    vec3 emissive = material.emissive * material.emissiveFactor;
    float metallic = material.metallicFactor;
    vec3 sampledNormal = inData.Normal;
    float roughness = material.roughnessFactor;

    if ((material.textureFlag & (1u << 0)) != 0u) {
        albedo *= texture(BaseColorMap, inData.TexCoord);
    }

    if ((material.textureFlag & (1u << 1)) != 0u) {
        sampledNormal = texture(NormalMap, inData.TexCoord).rgb;
        sampledNormal = normalize(sampledNormal * 2.0 - 1.0);
    }

    // Sample metallicRoughness map if enabled (Bit 2)
    if ((material.textureFlag & (1u << 2)) != 0u) { // Bit 2 for metallicRoughnessMap
        vec2 mr = texture(MetallicRoughnessMap, inData.TexCoord).rg;
        metallic = mr.r * material.metallicFactor;
        roughness = mr.g * material.roughnessFactor;
    }

    // Sample emissive map if enabled (Bit 3)
    if ((material.textureFlag & (1u << 3)) != 0u) { // Bit 3 for emissiveMap
        emissive += texture(EmissiveMap, inData.TexCoord).rgb;
    }
    
    float alphaCutoff = 0.5; // Set to 0.0 if not using alpha blending
    if (albedo.a < alphaCutoff) {
        discard;
    }

    // Compute TBN matrix for normal transformation
    vec3 interpTangent = normalize(inData.Tangent);
    vec3 interpNormal = normalize(inData.Normal);
    mat3 tbn = GetTBN(interpTangent,interpNormal);
    sampledNormal = tbn * sampledNormal;
    //sampledNormal = normalize(mix(interpNormal, sampledNormal,0.0));

    // Output to GBuffer
    OutAlbedoAlpha = albedo;
    OutNormal = sampledNormal;
    OutMetallicRoughness = vec2(metallic, roughness);
    OutEmissive = emissive;
}