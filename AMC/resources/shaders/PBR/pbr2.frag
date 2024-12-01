#version 460 core 

layout(binding = 0)uniform sampler2D texDiffuseColor;
layout(binding = 1)uniform sampler2D texNormal;
layout(binding = 2)uniform sampler2D texMettalicRoughness;
layout(binding = 4)uniform sampler2D texAmbientOcclusion;
layout(binding = 5)uniform sampler2D texEmission;

//Output
layout(location = 0)out vec4 FragColor;

//Input
in vec3 oNor;
in vec2 oTex;
in vec3 oWorldPos;
in mat3 oTbn;

//Material parameters
vec3 albedo;
float metallic;
float roughness;
float ao;

//Camera
uniform vec3 camPos;// = vec3(0.0f,0.0f,0.0f);

//Lights (Positional Light)
int numLights = 4;
vec3 lightPos[4] = {vec3(10.0,0.0f,0.0f),vec3(-10.0f,0.0f,0.0f),vec3(0.0f,10.0,0.0f),vec3(0.0f,-10.0f,0.0f)};
vec3 lightColor[4] = {vec3(1.0f,1.0f,1.0f),vec3(1.0f,0.0f,0.0f),vec3(0.0f,1.0f,0.0f),vec3(0.0f,0.0f,1.0f)};

const float PI = 3.14159265359;

//Helper Functions
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV,float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta,vec3 F0);

void main(void) 
{
    //Material values retrival
    albedo = pow(texture(texDiffuseColor,oTex).rgb,vec3(2.2));
    metallic  = texture(texMettalicRoughness, oTex).b;
    roughness = texture(texMettalicRoughness, oTex).g;
    ao = 1.0f;//texture(texAO, oTex).r;

    //Normal Calculation
    vec3 normal = vec3(texture(texNormal,oTex)) * 2.0f - 1.0f;
   // vec3 N = normalize(oTbn * normal);
    vec3 N = normalize(oNor);
    
    vec3 V = normalize(camPos - oWorldPos);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    //Reflectance equation
    vec3 Lo = vec3(0.0);

    for(int i = 0; i < numLights; i++)
    {
        vec3 L = normalize(lightPos[i] - oWorldPos);
        vec3 H = normalize(V+L);

        float distance = length(lightPos[i] - oWorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColor[i] * attenuation;

        //cook-torrance brdf
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);

        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS)* (1.0 - metallic);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N,V),0.0) * max(dot(N,L),0.0) + 0.000001;
        vec3 specular = numerator / denominator;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
    }
    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));
    color = pow(color,vec3(1.0/2.2));

    FragColor = vec4(color,1.0);
}


float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H),0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0)+1.0);
    denom = PI *denom *denom;
    return num /denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num/denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
   float NdotV = max(dot(N,V),0.0);
   float NdotL = max(dot(N,L),0.0);
   float ggx2 = GeometrySchlickGGX(NdotV,roughness);
    float ggx1 = GeometrySchlickGGX(NdotL,roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta ,0.0,1.0),5.0);
}

