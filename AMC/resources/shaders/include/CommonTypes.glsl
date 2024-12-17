struct PerFrameData
{
    mat4 ProjView;
    mat4 View;
    mat4 InvView;
    vec3 ViewPos;
    mat4 Projection;
    mat4 InvProjection;
    mat4 InvProjView;
    float NearPlane;
    float FarPlane;
};

// see https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_lights_punctual
#define MAX_LIGHTS 32
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