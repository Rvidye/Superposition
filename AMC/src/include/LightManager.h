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

            ShadowManager* GetShadowManager();

            void toggleLightShadow(Light& light, bool enable);

            void UpdateUBO();
            void UpdateShadows();

            void BindUBO();

            // For UI and debugging
            void renderUI();
            void drawLights();

        private:
            std::vector<Light> lights;
            ShadowManager *shadowManager = nullptr;
            GLuint uboLights = 0;
            static ShaderProgram* m_program;
            static Model *directional, *spot, *point;
    };
};
