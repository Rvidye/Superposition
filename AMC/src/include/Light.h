#pragma once

#include <glm/glm.hpp>

#define MAX_LIGHTS 32 // Same in Shader

namespace AMC {

    enum LightType {
        LIGHT_TYPE_DIRECTIONAL = 0,
        LIGHT_TYPE_SPOT,
        LIGHT_TYPE_POINT
    };

    class Light {
    public:
        bool active; // 1: active, 0 : inactive this avoided uncessary looping in shader
        int type; // 0 : Directional, 1 : Spot, 2: Point
        glm::vec3 color;
        float intensity;
        glm::vec3 position;
        glm::vec3 direction;
        float range;
        float spotAngle;     // in radians
        float spotExponent;
        bool shadows;
        int shadowIndex;     // assigned by ShadowManager

        Light()
            : active(false), type(LIGHT_TYPE_DIRECTIONAL), color(1.0f), intensity(1.0f),
            position(0.0f), direction(0.0f, -1.0f, 0.0f), range(100.0f),
            spotAngle(glm::radians(45.0f)), spotExponent(1.0f),
            shadows(false), shadowIndex(-1) {}
    };
};
