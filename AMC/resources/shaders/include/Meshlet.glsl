#define MESHLET_MAX_VERTEX_COUNT 128
#define MESHLET_MAX_TRIANGLE_COUNT 252

struct GpuMeshlet {
    uint VertexOffset;
    uint IndicesOffset;
    uint VertexCount;
    uint TriangleCount;
};

struct GpuMeshletInfo {
    vec3 Min;
    float _pad0;
    vec3 Max;
    float _pad1;
};

layout(std430, binding = 8) restrict readonly buffer VertexDataSSBO {
    Vertex vertices[];
} vertexDataSSBO;

layout(std430, binding = 11) restrict readonly buffer MeshletSSBO {
    GpuMeshlet Meshlets[];
} meshletSSBO;

layout(std430, binding = 12) restrict readonly buffer MeshletInfoSSBO {
    GpuMeshletInfo MeshletsInfo[];
} meshletInfoSSBO;

layout(std430, binding = 13) restrict readonly buffer MeshletVertexIndicesSSBO {
    uint VertexIndices[];
} meshletVertexIndicesSSBO;

layout(std430, binding = 14) restrict readonly buffer MeshletLocalIndicesSSBO {
    uint PackedIndices[];
} meshletLocalIndicesSSBO;
