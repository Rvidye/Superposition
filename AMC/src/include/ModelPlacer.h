#pragma once

#include<iostream>
#include<fstream>
#include<iomanip>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtx/euler_angles.hpp>
#ifdef _MYDEBUG
#include<imgui/imgui.h>
#endif

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

    ModelPlacer::ModelPlacer() : translatedBy(0.0f), rotateBy(0.0f), scaleBy(1.0f), multiplier(1.0f), mode(TRANSLATE) {}

    ModelPlacer::ModelPlacer(glm::vec3 t, glm::vec3 r, float s) : translatedBy(t), rotateBy(r), scaleBy(s), multiplier(1.0f), mode(TRANSLATE) {}

    glm::mat4 ModelPlacer::getModelMatrix() {
        glm::mat4 T = glm::translate(glm::mat4(1.0f), translatedBy);
        glm::mat4 R = glm::yawPitchRoll(rotateBy.y, rotateBy.x, rotateBy.z);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(scaleBy));
        return T * R * S; // Apply transformations in order: Translate -> Rotate -> Scale
    }

    void ModelPlacer::ApplyTranslation(char key) {
        switch (key) {
        case 'i': translatedBy.z -= multiplier; break;
        case 'k': translatedBy.z += multiplier; break;
        case 'j': translatedBy.x -= multiplier; break;
        case 'l': translatedBy.x += multiplier; break;
        case 'u': translatedBy.y -= multiplier; break;
        case 'o': translatedBy.y += multiplier; break;
        default: break;
        }
    }

    void ModelPlacer::ApplyRotation(char key) {
        switch (key) {
        case 'i': rotateBy.z -= glm::radians(multiplier); break;
        case 'k': rotateBy.z += glm::radians(multiplier); break;
        case 'j': rotateBy.x -= glm::radians(multiplier); break;
        case 'l': rotateBy.x += glm::radians(multiplier); break;
        case 'u': rotateBy.y -= glm::radians(multiplier); break;
        case 'o': rotateBy.y += glm::radians(multiplier); break;
        default: break;
        }
    }

    void ModelPlacer::ApplyScale(char key) {
        switch (key) {
        case 'i': scaleBy += multiplier; break;
        case 'k': scaleBy = std::max(0.1f, scaleBy - multiplier); break;
        default: break;
        }
    }

    void ModelPlacer::dump(){

        std::ofstream file("ModelTransform.txt", std::ios::app);

        if (!file.is_open()) {
            return;
        }

        file << std::fixed << std::setprecision(6);
        file << "glm::translate(glm::mat4(1.0f), glm::vec3("
             << translatedBy.x << "f, "
             << translatedBy.y << "f, "
             << translatedBy.z << "f)) * "
             << "glm::yawPitchRoll("
             << rotateBy.y << "f, "
             << rotateBy.x << "f, "
             << rotateBy.z << "f) * "
             << "glm::scale(glm::mat4(1.0f), glm::vec3("
             << scaleBy << "f));\n";
        file.close();
        std::cout << "Transformation dumped to ModelTransforms.txt\n";
    }

    void ModelPlacer::keyboardfunc(char key) {
        switch (key) {
        case 'b': mode = TRANSLATE; break;
        case 'n': mode = ROTATE; break;
        case 'm': mode = SCALE; break;
        case 'i': case 'k': case 'j': case 'l': case 'u': case 'o':
            if (mode == TRANSLATE) {
                ApplyTranslation(key);
            }
            else if (mode == ROTATE) {
                ApplyRotation(key);
            }
            else if (mode == SCALE) {
                ApplyScale(key);
            }
            break;
        case ',': multiplier *= 0.1f; break;
        case '.': multiplier *= 10.0f; break;
        default: break;
        }
    }

    void ModelPlacer::renderUI() {
        ImGui::Text("Select Mode:");
        if (ImGui::RadioButton("TRANSLATE", mode == TRANSLATE)) { mode = TRANSLATE; }
        ImGui::SameLine();
        if (ImGui::RadioButton("ROTATE", mode == ROTATE)) { mode = ROTATE; }
        ImGui::SameLine();
        if (ImGui::RadioButton("SCALE", mode == SCALE)) { mode = SCALE; }

        ImGui::Separator();

        ImGui::Text("Translation:");
        if (ImGui::DragFloat3("Position", &translatedBy[0], 0.1f)) {}

        ImGui::Separator();

        ImGui::Text("Rotation:");
        glm::vec3 degreesRotation = glm::degrees(rotateBy);
        if (ImGui::DragFloat3("Rotation (Degrees)", &degreesRotation[0], 1.0f)) {
            rotateBy = glm::radians(degreesRotation);
        }

        ImGui::Separator();

        ImGui::Text("Scale:");
        if (ImGui::DragFloat("Scale", &scaleBy, 0.1f, 0.1f, 10.0f)) {}

        ImGui::Separator();

        ImGui::Text("Multiplier: %.3f", multiplier);
        if (ImGui::Button("Increase (Period)")) { multiplier *= 10.0f; }
        ImGui::SameLine();
        if (ImGui::Button("Decrease (Comma)")) { multiplier *= 0.1f; }

        if (ImGui::Button("Dump ModelMatrix")) {
            dump();
        }

        ImGui::Text("To Move Selected Path Point");
        ImGui::Text("I/K = Z axis\nL/J = X axis\nO/U = Y axis");
        ImGui::Text("Multiplier: %.3f", multiplier);
    }

    std::ostream& operator<<(std::ostream& out, ModelPlacer* m) {
        out << "Translation: " << m->translatedBy.x << " " << m->translatedBy.y << " " << m->translatedBy.z << "\n";
        out << "Rotation: " << m->rotateBy.x << " " << m->rotateBy.y << " " << m->rotateBy.z << "\n";
        out << "Scale: " << m->scaleBy << "\n";
        return out;
    }

};
