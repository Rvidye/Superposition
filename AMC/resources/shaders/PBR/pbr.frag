#version 460 core



const float M_PI = 3.141592653589793;
// Constant normal incidence Fresnel factor for all dielectrics.
const vec3 Fdielectric = vec3(0.04);
const float Epsilon = 0.00001;


layout(binding = 0)uniform sampler2D texDiffuseColor;
layout(binding = 1)uniform sampler2D texNormal;
layout(binding = 2)uniform sampler2D texMettalicRoughness;
layout(binding = 4)uniform sampler2D texAmbientOcclusion;
layout(binding = 5)uniform sampler2D texEmission;

layout(binding = 6)uniform sampler2D texBRDF;
layout(binding = 7)uniform samplerCube texEnvDiffuse;
layout(binding = 8)uniform samplerCube texEnvSpecular;



struct MaterialInfo
{
    vec3 albedo;
    float metallic;
    float roughness;
    float emission;
    float alpha;
    float ao;
    int textureFlag;
};

// KHR_lights_punctual extension.
// see https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_lights_punctual
struct Light
{
    vec3 direction;
    float range;

    vec3 color;
    float intensity;

    vec3 position;
    float innerConeCos;

    float outerConeCos;
    int type;
};


const int LightType_Directional = 0;
const int LightType_Point = 1;
const int LightType_Spot = 2;


// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#range-property
float getRangeAttenuation(float range, float distance)
{
    if (range <= 0.0)
    {
        // negative range means unlimited
        return 1.0 / pow(distance, 2.0);
    }
    return max(min(1.0 - pow(distance / range, 4.0), 1.0), 0.0) / pow(distance, 2.0);
}


// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#inner-and-outer-cone-angles
float getSpotAttenuation(vec3 pointToLight, vec3 spotDirection, float outerConeCos, float innerConeCos)
{
    float actualCos = dot(normalize(spotDirection), normalize(-pointToLight));
    if (actualCos > outerConeCos)
    {
        if (actualCos < innerConeCos)
        {
            float angularAttenuation = (actualCos - outerConeCos) / (innerConeCos - outerConeCos);
            return angularAttenuation * angularAttenuation;
        }
        return 1.0;
    }
    return 0.0;
}


vec3 getLighIntensity(Light light, vec3 pointToLight)
{
    float rangeAttenuation = 1.0;
    float spotAttenuation = 1.0;

    if (light.type != LightType_Directional)
    {
        rangeAttenuation = getRangeAttenuation(light.range, length(pointToLight));
    }
    if (light.type == LightType_Spot)
    {
        spotAttenuation = getSpotAttenuation(pointToLight, light.direction, light.outerConeCos, light.innerConeCos);
    }

    return rangeAttenuation * spotAttenuation * light.intensity * light.color;
}

const int lightCount=1;

//Output
layout(location = 0)out vec4 FragColor;

uniform vec3 camPos;
uniform Light lights[lightCount]; //Array [0] is not allowed
uniform MaterialInfo materialInfo;

//Input
in vec3 oNor;
in vec2 oTex;
in vec3 oWorldPos;
in mat3 oTbn;


float ndfGGX(float cosLh, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;

    float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
    return alphaSq / (M_PI * denom * denom);
}

// Single term for separable Schlick-GGX below.
float gaSchlickG1(float cosTheta, float k)
{
    return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float gaSchlickGGX(float cosLi, float cosLo, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
    return gaSchlickG1(cosLi, k) * gaSchlickG1(cosLo, k);
}

// Shlick's approximation of the Fresnel factor.
vec3 fresnelSchlick(vec3 F0, float cosTheta)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// Returns number of mipmap levels for specular IBL environment map.
uint querySpecularTextureLevels()
{
    uint width, height, levels;
   // envSpecularTexture.GetDimensions(0, width, height, levels);
    return levels;
}

vec3 lerp(vec3 start,vec3 end,float t)
{
    return ((end - start) * t + start);
}

void main()
{
 
    vec3 tAlbedo = texture(texDiffuseColor,oTex).rgb; 
    
    vec3 normal = normalize(oNor);
    if (bool(materialInfo.textureFlag & (1<<1)))
    {
        vec3 normalMap = normalize(texture(texNormal, oTex).rgb * 2.0 - 1.0);
        normal = normalize(vec3(oTbn * normalMap));
    }
    
    float tMetallic = materialInfo.metallic;
    float tRoughness = materialInfo.roughness;
    if (bool(materialInfo.textureFlag & (1 << 2)))
    {
        vec4 mrSample = texture(texMettalicRoughness, oTex);
        tRoughness *= mrSample.g;
        tMetallic *= mrSample.b;
    }
    
    tRoughness = clamp(tRoughness,0.0,1.0);
    tMetallic = clamp(tMetallic,0.0,1.0);

    vec3 Lo = normalize(camPos.xyz - oWorldPos.xyz);
    float cosLo = max(0.0, dot(normal, Lo));
    vec3 Lr = 2.0 * cosLo * normal - Lo;
    vec3 F0 = lerp(Fdielectric, tAlbedo, tMetallic);
    vec3 directLight = vec3(0.0, 0.0, 0.0);
    
      
    for (int i = 0; i < lightCount; i++)
    {
        Light light = lights[i];

       // if (light.active == 0)
       //   continue;
		
        vec3 pointToLight;
        if (light.type != LightType_Directional)
        {
            pointToLight = light.position - oWorldPos;
        }
        else
        {
            pointToLight = -light.direction;
        }
        
        vec3 Li = -light.direction;
        vec3 Lradiance = light.color;
        
        vec3 lightIntensity = getLighIntensity(light,pointToLight);
        
        vec3 Lh = normalize(Li + Lo);
        
        // Calculate angles between surface normal and various light vectors.
        float cosLi = max(0.0, dot(normal, Li));
        float cosLh = max(0.0, dot(normal, Lh));
        
		// Calculate Fresnel term for direct lighting. 
        vec3 F = fresnelSchlick(F0, max(0.0, dot(Lh, Lo)));
		// Calculate normal distribution for specular BRDF.
        float D = ndfGGX(cosLh, tRoughness);
		// Calculate geometric attenuation for specular BRDF.
        float G = gaSchlickGGX(cosLi, cosLo, tRoughness);

        vec3 kd = lerp(vec3(1, 1, 1) - F, vec3(0.01, 0.01, 0.01), tMetallic);
        vec3 diffuseBRDF = kd * tAlbedo;
        vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * cosLo);
        directLight += (diffuseBRDF + specularBRDF) * lightIntensity * cosLi;
    }
    
    vec3 indirectLight = vec3(0.01, 0.01, 0.01);
    if (true)
    {
        vec3 irradiance = texture(texEnvDiffuse, normal).rgb;
        vec3 F = fresnelSchlick(F0, cosLo);
        vec3 kd = lerp(vec3(1, 1, 1) - F, vec3(0.01, 0.01, 0.01), tMetallic);
        vec3 diffuseIBL = kd * tAlbedo * irradiance;
        //uint specularTextureLevels = querySpecularTextureLevels();
        vec3 specularIrradiance = texture(texEnvSpecular, Lr).rgb;
        vec2 specularBRDF = texture(texBRDF, vec2(cosLo, tRoughness)).rg;
        vec3 specularIBL = (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;
        indirectLight = 0.5f * (diffuseIBL + specularIBL);
    }

    vec3 emissive = vec3(materialInfo.emission);
    if (bool(materialInfo.textureFlag & (1 << 3)))
    {
        emissive *= texture(texEmission,oTex).rgb;
    }

    float ao = 1.0;
    if (bool(materialInfo.textureFlag & (1 << 4)))
    {
        ao = texture(texAmbientOcclusion, oTex).r;
    }
    
    vec3 color = (directLight + indirectLight + emissive) * (ao);
    
    //finalColor += emissive;
    FragColor =  vec4(color, materialInfo.alpha); //(albedoTexture.Sample(SamplerLinear, input.texcoord).rgb, alpha);

//    FragColor =texture(texDiffuseColor,oTex);
};

