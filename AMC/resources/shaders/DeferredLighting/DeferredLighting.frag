#version 460 core

#extension GL_ARB_bindless_texture : require
#extension GL_EXT_shader_image_load_formatted : require

#include<..\..\..\resources\shaders\include\CommonTypes.glsl>
#include<..\..\..\resources\shaders\include\Transformations.glsl>
#include<..\..\..\resources\shaders\include\StaticUniformBuffers.glsl>
#include<..\..\..\resources\shaders\include\Compression.glsl>
#include<..\..\..\resources\shaders\include\Surface.glsl>
#include<..\..\..\resources\shaders\include\Pbr.glsl>

layout(location = 0) out vec4 OutFragColor;

layout(binding = 5) uniform sampler2D SamplerAO;
layout(binding = 6) uniform sampler2D SamplerIndirectLighting;

vec3 EvaluateLighting(Light light, Surface surface, vec3 fragPos, vec3 viewPos, float ambientOcclusion);
float Visibility(Shadows light, vec3 normal, vec3 lightToSample);
float GetLightSpaceDepth(Shadows light, vec3 lightSpaceSamplePos);

layout(location = 0) uniform bool IsShadows;
layout(location = 1) uniform bool IsVXGI;

in InOutData
{
    vec2 TexCoord;
} inData;

void main()
{
    ivec2 imgCoord = ivec2(gl_FragCoord.xy);
    vec2 uv = inData.TexCoord;

    // G-buffer values
    float depth = texelFetch(gBufferDataUBO.Depth, imgCoord, 0).r;
    if (depth == 1.0)
    {
        OutFragColor = vec4(0.0);
        return;
    }

    vec3 ndc = vec3(uv * 2.0 - 1.0, depth);
    vec3 fragPos = PerspectiveTransform(ndc, perFrameDataUBO.InvProjView);
    vec3 unjitteredFragPos = PerspectiveTransform(vec3(ndc.xy - vec2(0.0,0.0), ndc.z), perFrameDataUBO.InvProjView);

    vec3 albedo = texelFetch(gBufferDataUBO.AlbedoAlpha, imgCoord, 0).rgb;
    float alpha = texelFetch(gBufferDataUBO.AlbedoAlpha, imgCoord, 0).a;
    vec3 normal = DecodeUnitVec(texelFetch(gBufferDataUBO.Normal, imgCoord, 0).rg);
    float metallic = texelFetch(gBufferDataUBO.MetallicRoughness, imgCoord, 0).r;
    float roughness = texelFetch(gBufferDataUBO.MetallicRoughness, imgCoord, 0).g;
    //vec3 emissive = texelFetch(gBufferDataUBO.Emissive, imgCoord, 0).rgb; directly add in bloom pass
    float ambientOcclusion = 1.0 - texelFetch(SamplerAO, imgCoord, 0).r;

    Surface surface = GetDefaultSurface();
    surface.Albedo = albedo;
    surface.Normal = normal;
    surface.Metallic = metallic;
    surface.Roughness = roughness;
    surface.IOR = 1.0;

    vec3 directLighting = vec3(0.0);
    for (int i = 0; i < u_LightCount; i++)
    {
        Light light = u_Lights[i];
        if(light.isactive == 0) continue;

        vec3 contribution = EvaluateLighting(light, surface, fragPos, perFrameDataUBO.ViewPos, ambientOcclusion);

        if (contribution != vec3(0.0) && IsShadows)
        {
            float shadow = 0.0;
            if (light.shadowMapIndex == -1)
            {
                shadow = 0.0;
            }
            else
            {
                Shadows lightShadow = shadows[light.shadowMapIndex];
                vec3 lightToSample = unjitteredFragPos - light.position;

                // shadow = 1.0 - Visibility(lightShadow, normal, lightToSample);  <-- bug that caused `inverted' shadows
                shadow = Visibility(lightShadow, normal, lightToSample);  // bug fix
            }
            contribution *= (1.0 - shadow);
        }
        directLighting += contribution;
    }

    vec3 indirectLight;
    if(IsVXGI)
    {
        indirectLight = texelFetch(SamplerIndirectLighting, imgCoord, 0).rgb * albedo;
    }
    else
    {
        const vec3 ambient = vec3(0.015);
        indirectLight = ambient * albedo;
    }

    OutFragColor = vec4((directLighting + indirectLight), 1.0);
}

vec3 EvaluateLighting(Light light, Surface surface, vec3 fragPos, vec3 viewPos, float ambientOcclusion)
{
    vec3 surfaceToLight = light.position - fragPos;
    vec3 dirSurfaceToCam = normalize(viewPos - fragPos);
    vec3 dirSurfaceToLight = normalize(surfaceToLight);
    
    float distSq = dot(surfaceToLight, surfaceToLight);
    float attenuation = GetAttenuationFactor(distSq, light.range);

    
    const float prevIor = 1.0;
    vec3 fresnelTerm;
    vec3 specularBrdf = GGXBrdf(surface, dirSurfaceToCam, dirSurfaceToLight, prevIor, fresnelTerm);
    vec3 diffuseBrdf = surface.Albedo * ambientOcclusion;
    
    vec3 combinedBrdf = specularBrdf + diffuseBrdf * (vec3(1.0) - fresnelTerm) * (1.0 - surface.Metallic); 

    float cosTheta = clamp(dot(surface.Normal, dirSurfaceToLight), 0.0, 1.0);
    return combinedBrdf * attenuation * cosTheta * light.color;
}

float Visibility(Shadows light, vec3 normal, vec3 lightToSample)
{
    // TODO: Use overall better sampling method
    // Source: https://learnopengl.com/Advanced-Lighting/Shadows/Point-Shadows
    const vec3 ShadowSampleOffsets[] =
    {
        vec3( 0.0,  0.0,  0.0 ),
        vec3( 1.0,  1.0,  1.0 ), vec3(  1.0, -1.0,  1.0 ), vec3( -1.0, -1.0,  1.0 ), vec3( -1.0,  1.0,  1.0 ), 
        vec3( 1.0,  1.0, -1.0 ), vec3(  1.0, -1.0, -1.0 ), vec3( -1.0, -1.0, -1.0 ), vec3( -1.0,  1.0, -1.0 ),
        vec3( 1.0,  1.0,  0.0 ), vec3(  1.0, -1.0,  0.0 ), vec3( -1.0, -1.0,  0.0 ), vec3( -1.0,  1.0,  0.0 ),
        vec3( 1.0,  0.0,  1.0 ), vec3( -1.0,  0.0,  1.0 ), vec3(  1.0,  0.0, -1.0 ), vec3( -1.0,  0.0, -1.0 ),
        vec3( 0.0,  1.0,  1.0 ), vec3(  0.0, -1.0,  1.0 ), vec3(  0.0, -1.0, -1.0 ), vec3(  0.0,  1.0, -1.0 )
    };
    const float bias = 0.018;
    const float sampleDiskRadius = 0.04;

    float visibilityFactor = 0.0;
    for (int i = 0; i < ShadowSampleOffsets.length(); i++)
    {
        vec3 samplePos = (lightToSample + ShadowSampleOffsets[i] * sampleDiskRadius);
        float depth = GetLightSpaceDepth(light, samplePos * (1.0 - bias));
        visibilityFactor += texture(light.PcfShadowTexture, vec4(samplePos, depth));
    }
    visibilityFactor /= ShadowSampleOffsets.length();

    return visibilityFactor;
}

float GetLightSpaceDepth(Shadows light, vec3 lightSpaceSamplePos)
{
    float dist = max(abs(lightSpaceSamplePos.x), max(abs(lightSpaceSamplePos.y), abs(lightSpaceSamplePos.z)));
    float depth = GetLogarithmicDepth(light.NearPlane, light.FarPlane, dist);
    return depth;
}