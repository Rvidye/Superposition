
layout(std140, binding = 0) uniform PerFrameDataUBO
{
    PerFrameData perFrameDataUBO;
};

layout(std140, binding = 1) uniform LightBlock {
    Light u_Lights[MAX_LIGHTS];
    int u_LightCount;
    int pad0; int pad1; int pad2; // Padding for std140 alignment
};

layout(std140, binding = 4) uniform SkyBoxUBO{
    samplerCube Albedo;
}skyBoxUBO;

layout(std140, binding = 5) uniform VoxelizerDataUBO
{
    vec3 GridMin;
    float _pad0;
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
