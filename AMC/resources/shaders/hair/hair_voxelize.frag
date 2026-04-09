#version 460 core

#extension GL_EXT_shader_image_load_formatted : require
#extension GL_NV_shader_atomic_fp16_vector : require
#extension GL_NV_gpu_shader5 : require

layout(binding = 0, rgba16f) restrict uniform image3D ImgResult;

layout(std140, binding = 5) uniform VoxelizerDataUBO
{
    vec3 GridMin;
    float _pad0;
    vec3 GridMax;
    float _pad1;
} voxelizerDataUBO;

in VoxelData
{
    vec3 FragPos;
    vec3 Normal;
} inData;

ivec3 WorldSpaceToVoxelImageSpace(vec3 worldPos)
{
    vec3 uvw = (worldPos - voxelizerDataUBO.GridMin) / (voxelizerDataUBO.GridMax - voxelizerDataUBO.GridMin);
    return ivec3(clamp(uvw, vec3(0.0), vec3(0.999999)) * vec3(imageSize(ImgResult)));
}

void main()
{
    ivec3 voxelPos = WorldSpaceToVoxelImageSpace(inData.FragPos);
    float rootBias = 0.35 + 0.65 * abs(dot(normalize(inData.Normal), vec3(0.0, 1.0, 0.0)));
    vec3 hairColor = vec3(0.16, 0.10, 0.06) * rootBias + vec3(0.02);
    imageAtomicMax(ImgResult, voxelPos, f16vec4(hairColor, 1.0));
}
