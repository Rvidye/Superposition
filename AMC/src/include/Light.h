#pragma once

#include <glm/glm.hpp>
#include<UBO.h>

#define MAX_LIGHTS 32 // Same in Shader
#define MAX_SHADOWS 16 // Same in Shader

namespace AMC {

    enum LightType {
        LIGHT_TYPE_DIRECTIONAL = 0,
        LIGHT_TYPE_SPOT,
        LIGHT_TYPE_POINT
    };

    // ================================================================
    // Per-light shadow backend selection
    // ================================================================
    enum class ShadowBackend : uint8_t {
        None,              // No shadow for this light
        RasterShadowMap,   // Traditional cubemap shadow map
        RTShadow           // Vulkan ray-query shadow
    };

    class Light {
    public:
        GpuLight gpuLight;
        ShadowBackend shadowBackend = ShadowBackend::None;
        Light();
        ~Light();
        bool HasShadow() const;
        void ConnectShadow(int shadowIndex);
        void DisconnectShadow();
    };

    class Shadow {
        public:
            GLuint fbo = 0;
            GLuint texture = 0;
            bool needsUpdate = true;
            GpuShadow gpuShadow;
            glm::mat4 projection;

#if defined(_MYDEBUG)
            std::vector<GLuint> debugcubemapFaceViews;
#endif

            Shadow();
            ~Shadow();
            void UpdateViewMatrices();
    };
};
