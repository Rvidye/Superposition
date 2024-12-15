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

// Uniform block containing all light data
layout(std140, binding = 0) uniform LightBlock {
    Light u_Lights[MAX_LIGHTS];
    int u_LightCount;
    int pad0; int pad1; int pad2; // Padding for std140 alignment
};

// Shadow samplers
layout(binding = 1) uniform sampler2DArrayShadow u_ShadowMapArray; 
layout(binding = 2) uniform samplerCubeArrayShadow u_PointShadowArray;

// Function to calculate shadow factor for directional and spot lights
float calculateShadowDirectionalSpot(int lightIdx, vec4 fragPosLightSpace) {
    return texture(u_ShadowMapArray, fragPosLightSpace);
}

// Function to calculate shadow factor for point lights
float calculateShadowPoint(int lightIdx, vec3 lightToFragDir, float distance) {
    return texture(u_PointShadowArray, vec4(lightToFragDir, float(u_Lights[lightIdx].shadowMapIndex)), 0.0);
}