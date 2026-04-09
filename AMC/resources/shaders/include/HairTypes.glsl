const float HAIR_SENTINEL = 1.0 / 255.0;
const int HAIR_TOPOLOGY_QUAD = 0;
const int HAIR_TOPOLOGY_TRIANGLE = 1;

struct HairAssetGpuData
{
    ivec4 Counts;
    vec4 LODParams;
    vec4 FuzzParams;
    vec4 FrizzParams;
    vec4 KinkParams;
    vec4 CurlParams;
    vec4 StyleExtraParams;
    vec4 ClumpParams;
    sampler3D HairMeshTexture;
    sampler2D UVTexture;
    sampler3D WTexture;
    sampler3D UTexture;
    sampler3D VTexture;
};

struct HairBundleDesc
{
    vec3 BoundsMin;
    float _pad0;
    vec3 BoundsMax;
    float _pad1;
    vec4 AtlasRect;
    ivec4 SliceInfo;
    ivec4 RootInfo;
};

struct HairRootSample
{
    vec4 UVSeed;
};

struct HairSimulationGpuData
{
    vec4 GravityDeltaTime;
    vec4 WindDirectionTime;
    vec4 Dynamics;
    vec4 Limits;
};

struct HairSimulationPointGpuData
{
    vec4 Position;
    vec4 Velocity;
};

struct HairSimulationRestPointGpuData
{
    vec4 Position;
    vec4 UVW;
};

layout(std140, binding = 24) uniform HairAssetUBO
{
    HairAssetGpuData hairAsset;
} hairAssetUBO;

layout(std430, binding = 25) restrict readonly buffer HairBundleSSBO
{
    HairBundleDesc Bundles[];
} hairBundleSSBO;

layout(std430, binding = 26) restrict readonly buffer HairRootSampleSSBO
{
    HairRootSample Samples[];
} hairRootSampleSSBO;

layout(std140, binding = 27) uniform HairSimulationParamsUBO
{
    HairSimulationGpuData hairSimulation;
} hairSimulationUBO;

layout(std430, binding = 28) restrict readonly buffer HairSimulationReadSSBO
{
    HairSimulationPointGpuData ReadPoints[];
} hairSimulationReadSSBO;

layout(std430, binding = 29) restrict buffer HairSimulationWriteSSBO
{
    HairSimulationPointGpuData WritePoints[];
} hairSimulationWriteSSBO;

layout(std430, binding = 30) restrict readonly buffer HairSimulationRestSSBO
{
    HairSimulationRestPointGpuData RestPoints[];
} hairSimulationRestSSBO;
