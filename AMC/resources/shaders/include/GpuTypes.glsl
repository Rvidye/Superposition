// GpuTypes.glsl - GPU struct definitions for the global pooled buffer architecture
// These structs mirror C++ side definitions in ModelAssetManager.h

// ============================================================
// Immutable asset data (uploaded once at load time)
// ============================================================

struct GpuMeshRecord {
    uint  meshletOffset;        // into global meshlet pool
    uint  meshletCount;
    uint  materialId;           // into global material pool
    float _pad0;
    vec3  boundsMin;
    float _pad1;
    vec3  boundsMax;
    float _pad2;
};

struct GpuNodeRecord {
    int   parentIndex;          // -1 for root; local to asset's node range
    uint  meshBindingOffset;    // into global GpuMeshBinding pool
    uint  meshBindingCount;
    float _pad0;
    mat4  localTransform;       // rest-pose local transform
};

struct GpuMeshBinding {
    uint meshId;                // into global GpuMeshRecord pool
};

struct GpuModelAsset {
    uint nodeOffset;            // into global GpuNodeRecord pool
    uint nodeCount;
    uint meshOffset;            // into global GpuMeshRecord pool
    uint meshCount;
    uint meshBindingOffset;     // into global GpuMeshBinding pool
    uint meshBindingCount;
    uint materialOffset;        // into global material pool
    uint materialCount;
};

// ============================================================
// Dynamic per-instance data (updated every frame)
// ============================================================

struct GpuModelInstance {
    uint  assetId;              // into GpuModelAsset pool
    uint  transformBase;        // into transform palette
    uint  nodeCount;            // copied from asset for convenience
    uint  flags;                // bit 0: visible, bit 1: animated, bit 2: skeletal
};

// ============================================================
// Frame-local rendering data
// ============================================================

struct GpuRenderItem {
    uint  meshId;               // global mesh pool index
    uint  transformIndex;       // = instance.transformBase + nodeIndex
    uint  materialId;           // global material pool index
    uint  instanceId;           // back-reference to GpuModelInstance
    vec3  worldBoundsMin;       // transformed bounds for culling
    float _pad0;
    vec3  worldBoundsMax;
    float _pad1;
};

struct GpuMeshTaskCommand {
    uint count;                 // number of task shader workgroups
    uint first;                 // always 0 for NV path
    uint renderItemId;          // into render item pool
    uint _pad0;
};
