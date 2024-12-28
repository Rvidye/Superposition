#version 460 core

#extension GL_ARB_bindless_texture : require
#extension GL_EXT_shader_image_load_formatted : require

#extension GL_NV_shader_atomic_fp16_vector : require
#extension GL_NV_gpu_shader5 : require

#include<..\..\..\resources\shaders\include\CommonTypes.glsl>
#include<..\..\..\resources\shaders\include\Transformations.glsl>
#include<..\..\..\resources\shaders\include\StaticUniformBuffers.glsl>
#include<..\..\..\resources\shaders\include\Compression.glsl>
#include<..\..\..\resources\shaders\include\Surface.glsl>
#include<..\..\..\resources\shaders\include\Pbr.glsl>

layout(binding = 0, rgba16f) restrict uniform image3D ImgResult;

layout(location = 4)uniform int materialIndex;

layout (binding = 0)uniform sampler2D BaseColorMap;
layout (binding = 3)uniform sampler2D EmissiveMap;
layout(binding = 8) uniform samplerCubeArray SamplerPointShadowmap;

in InOutData
{
    vec3 FragPos;
    vec2 TexCoord;
    vec3 Normal;
} inData;

vec3 EvaluateDiffuseLighting(Light light, vec3 albedo, vec3 sampleToLight);
float Visibility(Shadows pointShadow, vec3 lightToSample);
float GetLightSpaceDepth(Shadows pointShadow, vec3 lightSpaceSamplePos);
ivec3 WorlSpaceToVoxelImageSpace(vec3 worldPos);

void main()
{
    ivec3 voxelPos = WorlSpaceToVoxelImageSpace(inData.FragPos);

    GpuMaterial material = materialSSBO.Materials[materialIndex];
    Surface surface = GetSurface(material, inData.TexCoord);
    surface.Emissive = surface.Emissive + surface.Albedo;

    vec3 directLighting = vec3(0.0);
    for (int i = 0; i < u_LightCount; i++)
    {
        Light light = u_Lights[i];
        vec3 sampleToLight = light.position - inData.FragPos;
        vec3 contrib = EvaluateDiffuseLighting(light, surface.Albedo, sampleToLight);
        if (light.shadowMapIndex >= 0)
        {
            Shadows lightShadow = shadows[light.shadowMapIndex];
            contrib *= Visibility(lightShadow, -sampleToLight);
        }
        directLighting += contrib;
    }

    const float ambient = 0.02;
    directLighting += surface.Albedo * ambient;
    directLighting += surface.Emissive;
    imageAtomicMax(ImgResult, voxelPos, f16vec4(directLighting, 1.0));
}

vec3 EvaluateDiffuseLighting(Light light, vec3 albedo, vec3 sampleToLight)
{
    float dist = length(sampleToLight);

    vec3 lightDir = sampleToLight / dist;
    float cosTheta = dot(normalize(inData.Normal), lightDir);
    if (cosTheta > 0.0)
    {
        vec3 diffuse = light.color * cosTheta * albedo;
        float attenuation = GetAttenuationFactor(dist * dist, light.range);

        return diffuse * attenuation;
    }

    return vec3(0.0);
}

float Visibility(Shadows light, vec3 lightToSample)
{
    float bias = 0.02;
    const float sampleDiskRadius = 0.08;

    float depth = GetLightSpaceDepth(light, lightToSample * (1.0 - bias));
    float visibilityFactor = texture(light.PcfShadowTexture, vec4(lightToSample, depth));

    return visibilityFactor;
}

float GetLightSpaceDepth(Shadows light, vec3 lightSpaceSamplePos)
{
    float dist = max(abs(lightSpaceSamplePos.x), max(abs(lightSpaceSamplePos.y), abs(lightSpaceSamplePos.z)));
    float depth = GetLogarithmicDepth(light.NearPlane, light.FarPlane, dist);

    return depth;
}

ivec3 WorlSpaceToVoxelImageSpace(vec3 worldPos)
{
    vec3 uvw = MapToZeroOne(worldPos, voxelizerDataUBO.GridMin, voxelizerDataUBO.GridMax);
    ivec3 voxelPos = ivec3(uvw * imageSize(ImgResult));
    return voxelPos;
}