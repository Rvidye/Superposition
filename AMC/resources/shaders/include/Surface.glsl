struct Surface {
    vec3 Albedo;
    float Alpha;

    vec3 Normal;
    vec3 Emissive;
    vec3 Absorbance;

    float Metallic;
    float Roughness; 
    float Transmission;
    float IOR;
    
    float AlphaCutoff;
};

Surface GetDefaultSurface()
{
    Surface surface;

    surface.Albedo = vec3(1.0);
    surface.Alpha = 1.0;

    surface.Normal = vec3(0.0);
    surface.Emissive = vec3(0.0);
    surface.Absorbance = vec3(0.0);

    surface.Metallic = 0.0;
    surface.Roughness = 0.0;
    surface.Transmission = 0.0;
    surface.IOR = 1.5;

    surface.AlphaCutoff = 0.5;
    
    return surface;
}

Surface GetSurface(GpuMaterial gpuMaterial, vec2 uv, float baseColorLodBias)
{
    Surface surface;
    vec4 baseColorAndAlpha = texture(gpuMaterial.BaseColor, uv, baseColorLodBias) * DecompressUR8G8B8A8(gpuMaterial.BaseColorFactor);
    surface.Albedo = baseColorAndAlpha.rgb;
    surface.Alpha = baseColorAndAlpha.a;

    surface.Normal = ReconstructPackedNormal(texture(gpuMaterial.Normal, uv).rg);
    surface.Emissive = texture(gpuMaterial.Emissive, uv).rgb * gpuMaterial.EmissiveFactor;
    surface.Absorbance = gpuMaterial.Absorbance;

    surface.Metallic = texture(gpuMaterial.MetallicRoughness, uv).r * gpuMaterial.MetallicFactor;
    surface.Roughness = texture(gpuMaterial.MetallicRoughness, uv).g * gpuMaterial.RoughnessFactor;
    surface.Transmission = texture(gpuMaterial.Transmission, uv).r * gpuMaterial.TransmissionFactor;
    surface.IOR = gpuMaterial.IOR;

    surface.AlphaCutoff = gpuMaterial.AlphaCutoff;

    return surface;
}

Surface GetSurface(GpuMaterial gpuMaterial, vec2 uv)
{
    Surface surface = GetSurface(gpuMaterial, uv, 0.0);
    return surface;
}

