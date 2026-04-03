#pragma once

#include<GL/glew.h>
#include<vector>
#include<Log.h>
#include<Light.h>
#include<ShadowManager.h>
#include<ShaderProgram.h>
#include<Model.h>
#include<glm/glm.hpp>

namespace AMC {

    class LightManager {
        public:

            LightManager();
            ~LightManager();

            bool AddLight(Light& l);
            void DeleteLight(int index);
            Light* GetLight(int index);
            int GetLightCount() const { return static_cast<int>(lights.size()); }

            ShadowManager* GetShadowManager();

            void toggleLightShadow(Light& light, bool enable);

            void UpdateUBO();
            void UpdateShadows();

            void BindUBO();

            // === Version tracking ===
            uint64_t GetLightVersion() const { return m_lightVersion; }

            // === Shadow backend queries ===
            // Returns true if any currently-enabled pass needs raster shadow maps
            bool NeedsRasterShadowMaps(bool vxgiEnabled, bool volumetricEnabled, bool rtShadowsAvailable) const;

            // Resolve per-light shadow backend based on current pass configuration.
            // Consumer-aware: if VXGI or volumetric need raster shadows, all lights
            // resolve to raster even if RT is available.
            void ResolveShadowBackends(bool rtShadowsAvailable,
                bool vxgiEnabled, bool volumetricEnabled, bool deferredWantsRT);

            // For UI and debugging
            void renderUI();
            void drawLights();

        private:
            std::vector<Light> lights;
            ShadowManager *shadowManager = nullptr;
            GLuint uboLights = 0;
            uint64_t m_lightVersion = 1;
            static ShaderProgram* m_program;
            static Model *directional, *spot, *point;
    };
};
