
layout(std140, binding = 0) uniform PerFrameDataUBO
{
    PerFrameData perFrameDataUBO;
};

layout(std140, binding = 1) uniform LightBlock {
    Light u_Lights[MAX_LIGHTS];
    int u_LightCount;
    int pad0; int pad1; int pad2;
};

layout(std430, binding = 2) restrict readonly buffer MaterialSSBO
{
    GpuMaterial Materials[];
} materialSSBO;

layout(std140, binding = 4) uniform SkyBoxUBO{
    samplerCube Albedo;
}skyBoxUBO;

layout(std140, binding = 5) uniform VoxelizerDataUBO
{
    vec3 GridMin;
    float _pad0; // for alignment i can use vec4 but this is easier for computing stuff.
    vec3 GridMax;
    float _pad1;
} voxelizerDataUBO;

layout(std140, binding = 6) uniform GBufferDataUBO
{
    sampler2D AlbedoAlpha;
    sampler2D Normal;
    sampler2D MetallicRoughness;
    sampler2D Emissive;
    sampler2D Depth;
} gBufferDataUBO;
