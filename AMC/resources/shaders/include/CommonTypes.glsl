
struct PerFrameData
{
    mat4 ProjView;
    mat4 View;
    mat4 InvView;
    mat4 Projection;
    mat4 InvProjection;
    mat4 InvProjView;
    vec3 ViewPos;
    float NearPlane;
    float FarPlane;
};

// see https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_lights_punctual
#define MAX_LIGHTS 32
#define MAX_SHADOWS 16
struct Light {
    vec3 position;       // 12 bytes
    float intensity;     // 4 bytes
    vec3 direction;      // 12 bytes
    float range;         // 4 bytes
    vec3 color;          // 12 bytes
    float spotAngle;     // 4 bytes
    float spotExponent;  // 4 bytes
    int type;            // 4 bytes
    int shadows;         // 4 bytes
    int shadowMapIndex;  // 4 bytes
    int isactive;          // 4 bytes
};

struct Shadows{
    mat4 ProjViewMatrices[6];
    vec3 Position;
    float NearPlane;
    float FarPlane;
    int LightIndex;
    samplerCube ShadowMapTexture;
    samplerCubeShadow PcfShadowTexture;
};

// Material == Surface
struct Material{
    vec3 albedo;
    vec3 emissive;
    float metallicFactor;
    float roughnessFactor;
    float emissiveFactor;
    float alpha;
    uint textureFlag;
};

struct GpuMaterial
{
    vec3 EmissiveFactor;
    uint BaseColorFactor;

    vec3 Absorbance;
    float IOR;
    float TransmissionFactor;
    float RoughnessFactor;
    float MetallicFactor;
    float AlphaCutoff;

    sampler2D BaseColor;
    sampler2D MetallicRoughness;
    sampler2D Normal;
    sampler2D Emissive;
    sampler2D Transmission;
    uvec2 _pad0;
};

struct Vertex {
    vec4 position; 
    uint normal;   
    uint tangent;   
    vec2 texCoords; 
    ivec4 boneIDs;  
    vec4 weights;  
};
