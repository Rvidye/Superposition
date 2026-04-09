vec3 SafeHairNormalize(vec3 value, vec3 fallback)
{
    float len2 = dot(value, value);
    if (len2 <= 1e-8)
    {
        return fallback;
    }
    return value * inversesqrt(len2);
}

vec3 HairShiftTangent(vec3 tangent, vec3 normal, float shift)
{
    return SafeHairNormalize(tangent + normal * shift, tangent);
}

float HairSpecularLobe(vec3 tangent, vec3 viewDir, vec3 lightDir, float exponent)
{
    vec3 halfVector = normalize(viewDir + lightDir);
    float dotTH = clamp(dot(tangent, halfVector), -1.0, 1.0);
    float sinTH = sqrt(max(1.0 - dotTH * dotTH, 0.0));
    float dirAtten = smoothstep(-1.0, 0.0, dotTH);
    return dirAtten * pow(sinTH, exponent);
}

vec3 HairAbsorption(vec3 albedo)
{
    return max(-log(max(albedo, vec3(0.05))), vec3(0.02));
}

vec3 HairShade(Light light, Surface surface, vec3 fragPos, vec3 viewPos, float ambientOcclusion)
{
    vec3 surfaceToLight = light.position - fragPos;
    vec3 lightDir = normalize(surfaceToLight);
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 tangent = SafeHairNormalize(surface.Normal, vec3(0.0, 1.0, 0.0));
    vec3 hairNormal = SafeHairNormalize(cross(tangent, cross(viewDir, tangent)),
        SafeHairNormalize(cross(tangent, vec3(0.0, 1.0, 0.0)), vec3(1.0, 0.0, 0.0)));

    float distSq = dot(surfaceToLight, surfaceToLight);
    float attenuation = GetAttenuationFactor(distSq, light.range);
    float perpendicularLight = sqrt(max(1.0 - dot(tangent, lightDir) * dot(tangent, lightDir), 0.0));

    float roughness = clamp(surface.Roughness, 0.05, 1.0);
    float primaryExponent = mix(96.0, 20.0, roughness);
    float secondaryExponent = mix(48.0, 10.0, roughness);
    float primary = HairSpecularLobe(HairShiftTangent(tangent, hairNormal, -0.18), viewDir, lightDir, primaryExponent);
    float secondary = HairSpecularLobe(HairShiftTangent(tangent, hairNormal, 0.32), viewDir, lightDir, secondaryExponent);

    vec3 absorption = HairAbsorption(surface.Albedo);
    vec3 diffuse = surface.Albedo * ambientOcclusion * perpendicularLight * 0.35;
    vec3 r = vec3(primary) * 0.65;
    vec3 trt = exp(-absorption * 1.5) * secondary * 0.45;

    return (diffuse + r + trt) * attenuation * light.color;
}
