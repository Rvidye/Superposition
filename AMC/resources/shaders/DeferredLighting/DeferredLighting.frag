#version 460 core

#include<..\..\..\resources\shaders\include\CommonTypes.glsl>
#include<..\..\..\resources\shaders\include\Transformations.glsl>
#include<..\..\..\resources\shaders\include\StaticUniformBuffers.glsl>
#include<..\..\..\resources\shaders\include\Surface.glsl>
#include<..\..\..\resources\shaders\include\Pbr.glsl>

layout(location = 0) out vec4 OutFragColor;

layout(binding = 0) uniform sampler2D gAlbedoAlpha;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gMetallicRoughness;
layout(binding = 3) uniform sampler2D gEmissive;
layout(binding = 4) uniform sampler2D gDepth;
layout(binding = 5) uniform sampler2D SamplerAO;
layout(binding = 6) uniform sampler2D SamplerIndirectLighting;

vec3 EvaluateLighting(Light light, Surface surface, vec3 fragPos, vec3 viewPos, float ambientOcclusion);
//float Visibility(vec3 normal, vec3 lightToSample);
//float GetLightSpaceDepth(vec3 lightSpaceSamplePos);

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
    float depth = texelFetch(gDepth, imgCoord, 0).r;

    if (depth >= 1.0)
    {
        OutFragColor = vec4(0.0);
        return;
    }

    vec3 ndc = vec3(uv * 2.0 - 1.0, depth);
    vec3 fragPos = PerspectiveTransform(ndc, perFrameDataUBO.InvProjView);

    vec4 albedoAlpha = texelFetch(gAlbedoAlpha, imgCoord, 0);
    vec3 albedo = albedoAlpha.rgb;
    float alpha = albedoAlpha.a;

    vec3 normal = normalize(texelFetch(gNormal, imgCoord, 0).rgb);
    vec2 metallicRoughness = texelFetch(gMetallicRoughness, imgCoord, 0).rg;
    float metallic = metallicRoughness.r;
    float roughness = metallicRoughness.g;
    vec3 emissive = texelFetch(gEmissive, imgCoord, 0).rgb;
    float ambientOcclusion = 1.0 - texelFetch(SamplerAO, imgCoord, 0).r;

    Surface surface = GetDefaultSurface();
    surface.Albedo = albedo;
    surface.Normal = normal;
    surface.Metallic = metallic;
    surface.Roughness = roughness;

    vec3 directLighting = vec3(0.0);
    for (int i = 0; i < u_LightCount; i++)
    {
        Light light = u_Lights[i];
        vec3 contribution = EvaluateLighting(light, surface, fragPos, perFrameDataUBO.ViewPos, ambientOcclusion);

        // if (contribution != vec3(0.0))
        // {
        //     float shadow = 0.0;
        //     if (light.PointShadowIndex == -1)
        //     {
        //         shadow = 0.0;
        //     }
        //     else if (ShadowMode == SHADOW_MODE_PCF_SHADOW_MAP)
        //     {
        //         GpuPointShadow pointShadow = shadowsUBO.PointShadows[light.PointShadowIndex];
        //         vec3 lightToSample = unjitteredFragPos - light.Position;
        //         shadow = 1.0 - Visibility(pointShadow, normal, lightToSample);
        //     }
        //     else if (ShadowMode == SHADOW_MODE_RAY_TRACED)
        //     {
        //         GpuPointShadow pointShadow = shadowsUBO.PointShadows[light.PointShadowIndex];
        //         shadow = imageLoad(image2D(pointShadow.RayTracedShadowMapImage), imgCoord).r;
        //     }
        //     contribution *= (1.0 - shadow);
        // }
        directLighting += contribution;
    }

    vec3 indirectLight;
    // if (IsVXGI)
    // {
    //     indirectLight = texelFetch(SamplerIndirectLighting, imgCoord, 0).rgb * albedo;
    // }
    // else
    // {
        const vec3 ambient = vec3(0.015);
        indirectLight = ambient * albedo;
    // }

    OutFragColor = vec4((directLighting + indirectLight) + emissive, 1.0);
}

vec3 EvaluateLighting(Light light, Surface surface, vec3 fragPos, vec3 viewPos, float ambientOcclusion)
{
    vec3 dirSurfaceToCam = normalize(viewPos - fragPos);
    vec3 dirSurfaceToLight;

    float cosTheta = 0.0;
    float attenuation = 1.0;
    if (light.type == 0) {
        dirSurfaceToLight = normalize(-light.direction); 
        attenuation = 1.0; 
        cosTheta = max(dot(surface.Normal, dirSurfaceToLight), 0.0);
    } else if (light.type == 2) {
        // Point Light
        vec3 surfaceToLight = (light.position - fragPos);
        float distSq = dot(surfaceToLight, surfaceToLight);
        dirSurfaceToLight = normalize(surfaceToLight);
        attenuation = GetAttenuationFactor(distSq, light.range);
        cosTheta = max(dot(surface.Normal, dirSurfaceToLight), 0.0);
    }else if (light.type == 1) {
        vec3 surfaceToLight = light.position - fragPos;
        float distSq = dot(surfaceToLight, surfaceToLight);
        dirSurfaceToLight = normalize(surfaceToLight);
        attenuation = GetAttenuationFactor(distSq, light.range);
        // Compute spot effect
        float spotCos = max(dot(dirSurfaceToLight, normalize(-light.direction)), 0.0);
        float spotThreshold = cos(radians(light.spotAngle * 0.5)); 
        if (spotCos < spotThreshold) {
            // Outside the spot cone, no lighting
            return vec3(0.0);
        }
        // Smooth the edges using the spotExponent
        float spotFalloff = pow(spotCos, light.spotExponent);
        attenuation *= spotFalloff;

        cosTheta = max(dot(surface.Normal, dirSurfaceToLight), 0.0);
    }

    // Compute BRDF (specular + diffuse)
    const float prevIor = 1.0;
    vec3 fresnelTerm;
    vec3 specularBrdf = GGXBrdf(surface, dirSurfaceToCam, dirSurfaceToLight, prevIor, fresnelTerm);
    vec3 diffuseBrdf = surface.Albedo * ambientOcclusion;
    vec3 combinedBrdf = specularBrdf + diffuseBrdf * (vec3(1.0) - fresnelTerm) * (1.0 - surface.Metallic); 
    return combinedBrdf * attenuation * cosTheta * light.color;
}
/*
float Visibility(vec3 normal, vec3 lightToSample)
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
        float depth = GetLightSpaceDepth(pointShadow, samplePos * (1.0 - bias));
        visibilityFactor += texture(pointShadow.PcfShadowTexture, vec4(samplePos, depth));
    }
    visibilityFactor /= ShadowSampleOffsets.length();

    return visibilityFactor;
}

float GetLightSpaceDepth(vec3 lightSpaceSamplePos)
{
    float dist = max(abs(lightSpaceSamplePos.x), max(abs(lightSpaceSamplePos.y), abs(lightSpaceSamplePos.z)));
    float depth = GetLogarithmicDepth(pointShadow.NearPlane, pointShadow.FarPlane, dist);
    return depth;
}
*/
