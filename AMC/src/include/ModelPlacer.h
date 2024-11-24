#pragma once

#include<iostream>
#include<fstream>
#include<iomanip>
#include<glm/glm.hpp>
#include<glm/ext.hpp>
#ifdef _MYDEBUG
#include<imgui/imgui.h>
#endif
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace AMC {

    enum TransformMode {
        TRANSLATE,
        ROTATE,
        SCALE
    };

    class ModelPlacer {

        glm::vec3 translatedBy;
        glm::vec3 rotateBy; // Store rotation angles (pitch, yaw, roll)
        float scaleBy;
        float multiplier;
        TransformMode mode;

        void ApplyTranslation(char key);
        void ApplyRotation(char key);
        void ApplyScale(char key);
        void dump();

    public:
        ModelPlacer();
        ModelPlacer(glm::vec3 t, glm::vec3 r, float s);
        glm::mat4 getModelMatrix();
        void keyboardfunc(char key);
        void renderUI();
        friend std::ostream& operator<<(std::ostream& out, ModelPlacer* m);
    };
};
