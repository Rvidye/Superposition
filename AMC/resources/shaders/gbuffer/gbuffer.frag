#version 460 core

#include<..\..\..\resources\shaders\include\StaticUniformBuffers.glsl>

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
    vec3 albedo = material.albedo;
    vec3 emissive = material.emissive;
    float metallic = material.metallicFactor;
    float roughness = material.roughnessFactor;
    float alpha = material.alpha;

    if ((material.textureFlag & (1u << 0)) != 0u) {
        albedo *= texture(BaseColorMap, inData.TexCoord).rgb;
    }

    vec3 normal = normalize(inData.Normal);
    if ((material.textureFlag & (1u << 1)) != 0u) { // Bit 1 for NormalMap
        vec3 sampledNormal = texture(NormalMap, inData.TexCoord).rgb;
        sampledNormal = normalize(sampledNormal * 2.0 - 1.0); // Decode from [0,1] to [-1,1]
        normal = normalize(sampledNormal);
    }

    // Sample metallicRoughness map if enabled (Bit 2)
    if ((material.textureFlag & (1u << 2)) != 0u) { // Bit 2 for metallicRoughnessMap
        vec2 mr = texture(MetallicRoughnessMap, inData.TexCoord).rg;
        metallic = mr.r * material.metallicFactor;
        roughness = mr.g * material.roughnessFactor;
    }

    // Sample emissive map if enabled (Bit 3)
    if ((material.textureFlag & (1u << 3)) != 0u) { // Bit 3 for emissiveMap
        emissive *= texture(EmissiveMap, inData.TexCoord).rgb;
    }

    // Handle alpha cutoff (Modify as needed)
    float alphaCutoff = 0.0; // Set to 0.0 if not using alpha blending
    if (alpha < alphaCutoff) {
        discard;
    }

    // Compute TBN matrix for normal transformation
    vec3 tangent = normalize(inData.Tangent);
    vec3 bitangent = cross(normal, tangent);
    mat3 tbn = mat3(tangent, bitangent, normal);
    // Transform the normal using the TBN matrix
    vec3 transformedNormal = normalize(tbn * normal);

    // Output to GBuffer
    OutAlbedoAlpha = vec4(albedo, alpha);
    OutNormal = transformedNormal;
    OutMetallicRoughness = vec2(metallic, roughness);
    OutEmissive = emissive;
}