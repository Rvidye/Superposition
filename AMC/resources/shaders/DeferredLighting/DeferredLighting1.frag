#version 460 core
#define PI 3.14159265
#define FLOAT_MAX 3.4028235e+38
#define FLOAT_MIN -FLOAT_MAX
#define FLOAT_NAN (0.0 / 0.0)
#include<..\..\..\resources\shaders\include\CommonTypes.glsl>
#include<..\..\..\resources\shaders\include\Transformations.glsl>
#include<..\..\..\resources\shaders\include\StaticUniformBuffers.glsl>
#include<..\..\..\resources\shaders\include\Surface.glsl>

layout(location = 0) out vec4 OutFragColor;

layout(binding = 0) uniform sampler2D gAlbedoAlpha;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gMetallicRoughness;
layout(binding = 3) uniform sampler2D gEmissive;
layout(binding = 4) uniform sampler2D gDepth;
layout(binding = 5) uniform sampler2D SamplerAO;
layout(binding = 6) uniform sampler2D SamplerIndirectLighting;

//vec3 EvaluateLighting(Light light, Surface surface, vec3 fragPos, vec3 viewPos, float ambientOcclusion);
//float Visibility(vec3 normal, vec3 lightToSample);
//float GetLightSpaceDepth(vec3 lightSpaceSamplePos);

layout(location = 1) uniform bool IsVXGI;

in InOutData
{
    vec2 TexCoord;
} inData;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom,1e-5);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / max(denom,1e-5);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
} 

float getSquareFalloffAttenuation(vec3 pos_to_light, float light_inv_radius){

    float distance_square = dot(pos_to_light,pos_to_light);
    float factor = distance_square * light_inv_radius * light_inv_radius;
    float smooth_factor = max(1.0 - factor * factor,0.0);
    return (smooth_factor * smooth_factor) / max(distance_square,1e-5);
}

float getSpotAngleAttenuation(vec3 l, vec3 light_dir,float inner_angle,float outer_angle){

    float cos_outer = cos(outer_angle);
    float spot_scale = 1.0 / max(cos(inner_angle) - cos_outer,1e-5);
    float spot_offset = -cos_outer * spot_scale;
    float cd = dot(normalize(-light_dir),l);
    float attenuation = clamp(cd * spot_scale + spot_offset, 0.0,1.0);
    return attenuation;
}

vec3 pbr(Light light, Surface surface, vec3 fragPos, vec3 viewPos, float ambientOcclusion){

    if(light.isactive == 0) return vec3(0.0);

    vec3 w0 = normalize(viewPos - fragPos);
    vec3 wi;
    float attenuation = 1.0;

    // Determine light type
    if (light.type == 0) {  // Directional light
        wi = normalize(-light.direction);
    } else if (light.type == 1) {  // Point light
        vec3 lightDir = light.position - fragPos;
        attenuation = getSquareFalloffAttenuation(lightDir, 1.0 / max(light.range, 1e-5));
        wi = normalize(lightDir);
    } else if (light.type == 2) {  // Spot light
        vec3 l = normalize(fragPos - light.position);
        float spotAttenuation = getSpotAngleAttenuation(l, -light.direction, light.spotAngle, light.spotExponent);
        vec3 lightDir = light.position - fragPos;
        attenuation = getSquareFalloffAttenuation(lightDir, 1.0 / max(light.range, 1e-5)) * spotAttenuation;
        wi = normalize(lightDir);
    } else {
        return vec3(0.0);
    }

    vec3 h  = normalize(w0 + wi);
    vec3 radiance = light.color * light.intensity;

    // fresnel reflectance
    vec3 F0 = vec3(0.04);
    F0 = mix(F0,surface.Albedo,surface.Metallic);
    vec3 F = fresnelSchlick(max(dot(h, w0), 0.0), F0);

    // cook-torrance brdf
    float NDF = DistributionGGX(surface.Normal, h, surface.Roughness);   
    float G   = GeometrySmith(surface.Normal, w0, wi, surface.Roughness);

    float NdotL = max(dot(surface.Normal,wi),0.0);
    vec3 num = NDF * G * F;

    float denom = 4.0 * max(dot(surface.Normal,w0),0.0) * NdotL;
    vec3 specular = num / max(denom,1e-5);

    vec3 ks = F;
    vec3 kd = 1.0 - ks;
    kd = kd * (1.0 - surface.Metallic);

    return (kd * surface.Albedo / PI + specular) * radiance * NdotL * attenuation;
}

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
        vec3 contribution = pbr(light, surface, fragPos, normalize(perFrameDataUBO.ViewPos), 0.0);
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

    OutFragColor = vec4((directLighting) + emissive, 1.0);
}

