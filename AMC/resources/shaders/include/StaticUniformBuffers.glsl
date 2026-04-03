
layout(std140, binding = 0) uniform PerFrameDataUBO
{
    PerFrameData perFrameDataUBO;
};

layout(std140, binding = 1) uniform LightBlock {
    Light u_Lights[MAX_LIGHTS];
    int u_LightCount;
    int pad0; int pad1; int pad2;
};

layout(std140, binding = 2) uniform ShadowsUBO {
    Shadows shadows[MAX_SHADOWS];
    int count;
    float Padding[3];
};

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
    sampler2D Velocity;
} gBufferDataUBO;

layout(std140, binding = 7) uniform TAADataBlock
{
    TAAData taaDataUBO;
};

// SSBO's

layout(std430, binding = 2) restrict readonly buffer MaterialSSBO
{
    GpuMaterial Materials[];
} materialSSBO;

// skining
layout(std430, binding = 0) readonly buffer VertexBufferIn {
    Vertex vin[];
};

layout(std430, binding = 1) writeonly buffer VertexBufferOut {
    Vertex vout[];
};

layout(std430, binding = 6) readonly buffer BoneMatricesBuffer {
    mat4 boneMatrices[];
};

// ============================================================
// Global pool SSBOs for pooled-buffer architecture
// Guarded by USE_GLOBAL_POOLS define so existing shaders are unaffected.
// ============================================================

#ifdef USE_GLOBAL_POOLS

#include<..\..\..\resources\shaders\include\GpuTypes.glsl>

layout(std430, binding = 15) restrict readonly buffer MeshRecordPoolSSBO {
    GpuMeshRecord MeshRecords[];
} meshRecordPoolSSBO;

layout(std430, binding = 16) restrict readonly buffer NodeRecordPoolSSBO {
    GpuNodeRecord NodeRecords[];
} nodeRecordPoolSSBO;

layout(std430, binding = 17) restrict readonly buffer MeshBindingPoolSSBO {
    GpuMeshBinding MeshBindings[];
} meshBindingPoolSSBO;

layout(std430, binding = 18) restrict readonly buffer AssetPoolSSBO {
    GpuModelAsset Assets[];
} assetPoolSSBO;

// Transform palettes (Milestone 2)
layout(std430, binding = 19) restrict readonly buffer TransformPaletteSSBO {
    mat4 Transforms[];
} transformPaletteSSBO;

layout(std430, binding = 20) restrict readonly buffer PrevTransformPaletteSSBO {
    mat4 Transforms[];
} prevTransformPaletteSSBO;

layout(std430, binding = 21) restrict readonly buffer InstancePoolSSBO {
    GpuModelInstance Instances[];
} instancePoolSSBO;

#endif // USE_GLOBAL_POOLS
