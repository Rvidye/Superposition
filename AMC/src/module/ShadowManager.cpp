#pragma once

#include<ShadowManager.h>
#include<ShaderProgram.h>
#include<Scene.h>

static const int SHADOWMAP_SIZE = 1024;

namespace AMC {

    ShadowManager::ShadowManager(int maxDirSpot, int maxPoint) : maxShadowmaps(maxDirSpot), maxPointShadowcubemaps(maxPoint)
    {
        // Create 2D array for directional/spot shadows
        glCreateTextures(GL_TEXTURE_2D, 1, &shadowmap);
        glTextureStorage2D(shadowmap, 1, GL_DEPTH_COMPONENT32F, SHADOWMAP_SIZE, SHADOWMAP_SIZE);
        glTextureParameteri(shadowmap, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(shadowmap, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(shadowmap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(shadowmap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        //glTextureParameteri(shadowmap, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        //glTextureParameteri(shadowmap, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

        // Create cube map array for point lights
        glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, &pointShadowCubemap);
        glTextureStorage3D(pointShadowCubemap, 1, GL_DEPTH_COMPONENT32F, SHADOWMAP_SIZE, SHADOWMAP_SIZE, 6 * 3);
        glTextureParameteri(pointShadowCubemap, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(pointShadowCubemap, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(pointShadowCubemap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(pointShadowCubemap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(pointShadowCubemap, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        //glTextureParameteri(pointShadowCubemap, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        //glTextureParameteri(pointShadowCubemap, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

        glCreateFramebuffers(1, &shadowmapFBO);
        glNamedFramebufferTexture(shadowmapFBO, GL_DEPTH_ATTACHMENT, shadowmap, 0);
        glNamedFramebufferDrawBuffer(shadowmapFBO, GL_NONE);
        glNamedFramebufferReadBuffer(shadowmapFBO, GL_NONE);
        GLenum status = glCheckNamedFramebufferStatus(shadowmapFBO, GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            LOG_ERROR(L"Shadowmap FBO is not complete\n");
        }

        glCreateFramebuffers(1, &pointShadowmapFBO);
        glNamedFramebufferTexture(pointShadowmapFBO, GL_DEPTH_ATTACHMENT, pointShadowCubemap, 0);
        glNamedFramebufferDrawBuffer(pointShadowmapFBO, GL_NONE);
        glNamedFramebufferReadBuffer(pointShadowmapFBO, GL_NONE);
        status = glCheckNamedFramebufferStatus(pointShadowmapFBO, GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            LOG_ERROR(L"Point Shadowmap FBO is not complete\n");
        }

#if defined(_MYDEBUG)
        
        //if (debugShadowmapView.empty())
        //{
        //    debugShadowmapView.resize(maxShadowmaps);
        //    glGenTextures(maxShadowmaps, debugShadowmapView.data());

        //    for (int i = 0; i < maxShadowmaps; ++i)
        //    {
        //        glTextureView(debugShadowmapView[i], GL_TEXTURE_2D, shadowmap, GL_DEPTH_COMPONENT32F, 0, 1, i, 1);
        //    }
        //}

        if (debugcubemapFaceViews.empty())
        {
            debugcubemapFaceViews.resize(6 * maxPointShadowcubemaps);
            glGenTextures(6 * maxPointShadowcubemaps, debugcubemapFaceViews.data());

            for (int i = 0; i < maxPointShadowcubemaps; ++i)
            {
                for (int face = 0; face < 6; ++face)
                {
                    int index = i * 6 + face;
                    glTextureView(debugcubemapFaceViews[index], GL_TEXTURE_2D, pointShadowCubemap, GL_DEPTH_COMPONENT32F, 0, 1, i * 6 + face, 1);
                }
            }
        }
#endif

    }

    void ShadowManager::createShadowMapForLight(Light& light)
    {
        static int usedDirectionalSpot = 0;
        static int usedPoint = 0;
        if (light.type == LIGHT_TYPE_DIRECTIONAL || light.type == LIGHT_TYPE_SPOT) {
            if (usedDirectionalSpot < maxShadowmaps) {
                light.shadowIndex = usedDirectionalSpot++;
            }
            else {
                LOG_ERROR(L"No more directional/spot shadow layers available.\n");
                light.shadows = false;
            }
        }
        else if (light.type == LIGHT_TYPE_POINT) {
            if (usedPoint < maxPointShadowcubemaps) {
                light.shadowIndex = usedPoint++;
            }
            else {
                LOG_ERROR(L"No more point shadow layers available.\n");
                light.shadows = false;
            }
        }
    }

    void ShadowManager::removeShadowMapForLight(Light& light)
    {
        // Ideally should be a better mechanism for this but for now its okay.
        light.shadows = false;
        light.shadowIndex = -1;
    }

    void ShadowManager::renderShadowMaps(ShaderProgram* program, const Scene* scene)
    {
        // Compute all directional and spot light matrices and store in buffer
        auto lights = scene->lightManager->getShadowCastingLights();
        std::vector<glm::mat4> dirSpotMatrices;
        dirSpotMatrices.reserve(3);

        const AABB& sceneAABB = scene->sceneAABB;

        for (auto* light : lights) {
            if (!light->shadows) continue;
            if (light->type == LIGHT_TYPE_POINT) continue;
            glm::mat4 lightView = glm::lookAt(light->position, light->position + light->direction, glm::vec3(0, 1, 0));

            if (light->type == LIGHT_TYPE_DIRECTIONAL) {

                // too complex 
                glm::vec3 vertices[8] = {
                    glm::vec3(sceneAABB.mMin.x, sceneAABB.mMin.y, sceneAABB.mMin.z),
                    glm::vec3(sceneAABB.mMax.x, sceneAABB.mMin.y, sceneAABB.mMin.z),
                    glm::vec3(sceneAABB.mMin.x, sceneAABB.mMax.y, sceneAABB.mMin.z),
                    glm::vec3(sceneAABB.mMax.x, sceneAABB.mMax.y, sceneAABB.mMin.z),
                    glm::vec3(sceneAABB.mMin.x, sceneAABB.mMin.y, sceneAABB.mMax.z),
                    glm::vec3(sceneAABB.mMax.x, sceneAABB.mMin.y, sceneAABB.mMax.z),
                    glm::vec3(sceneAABB.mMin.x, sceneAABB.mMax.y, sceneAABB.mMax.z),
                    glm::vec3(sceneAABB.mMax.x, sceneAABB.mMax.y, sceneAABB.mMax.z),
                };

                glm::vec3 lightSpaceMin(FLT_MAX);
                glm::vec3 lightSpaceMax(-FLT_MAX);

                for (const auto& vertex : vertices) {
                    glm::vec3 lightSpaceVertex = glm::vec3(lightView * glm::vec4(vertex, 1.0f));
                    lightSpaceMin = glm::min(lightSpaceMin, lightSpaceVertex);
                    lightSpaceMax = glm::max(lightSpaceMax, lightSpaceVertex);
                }

                // not perfect but will do job for now
                //glm::vec3 lightDir = glm::normalize(light->direction);
                //float minZ = glm::dot(sceneAABB.mMin, lightDir);
                //float maxZ = glm::dot(sceneAABB.mMax, lightDir);
                //float left = sceneAABB.mMin.x;
                //float right = sceneAABB.mMax.x;
                //float bottom = sceneAABB.mMin.y;
                //float top = sceneAABB.mMax.y;

                glm::mat4 lightProj = glm::ortho(lightSpaceMin.x, lightSpaceMax.x, lightSpaceMin.y, lightSpaceMax.y, -lightSpaceMax.z, -lightSpaceMin.z);
                dirSpotMatrices.push_back(lightProj * lightView);
            }
            else {
                glm::mat4 lightProj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
                dirSpotMatrices.push_back(lightProj * lightView);
            }
        }

        float depthClear = 1.0f;
        glClearNamedFramebufferfv(shadowmapFBO, GL_DEPTH, 0, &depthClear);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowmapFBO);
        glViewport(0, 0, SHADOWMAP_SIZE, SHADOWMAP_SIZE);
        program->use();
        glUniformMatrix4fv(program->getUniformLocation("viewProj[0]"), 3, GL_FALSE, glm::value_ptr(dirSpotMatrices[0]));
        glUniform1i(program->getUniformLocation("numShadows"), (GLint)dirSpotMatrices.size());
        for (const auto& [name, obj] : scene->models) {

            if (!obj.visible)
                continue;
            glUniformMatrix4fv(program->getUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(obj.matrix));
            obj.model->draw(program,obj.numInstance,false);
        }
    }

    void ShadowManager::renderPointShadowMaps(ShaderProgram* program, const Scene* scene)
    {
        float depthClear = 1.0f;
        glClearNamedFramebufferfv(pointShadowmapFBO, GL_DEPTH, 0, &depthClear);
        glBindFramebuffer(GL_FRAMEBUFFER, pointShadowmapFBO);
        glViewport(0, 0, SHADOWMAP_SIZE, SHADOWMAP_SIZE);
        program->use();
        auto lights = scene->lightManager->getShadowCastingLights();

        for (auto* light : lights) {
            if (!light->active) continue;
            if (!light->shadows) continue;
            if (light->type != LIGHT_TYPE_POINT) continue;

            glm::mat4 lightProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.15f, 60.0f);

            std::vector<glm::mat4> shadowTransforms;
            shadowTransforms.push_back(lightProj * glm::lookAt(light->position, light->position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
            shadowTransforms.push_back(lightProj * glm::lookAt(light->position, light->position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
            shadowTransforms.push_back(lightProj * glm::lookAt(light->position, light->position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
            shadowTransforms.push_back(lightProj * glm::lookAt(light->position, light->position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
            shadowTransforms.push_back(lightProj * glm::lookAt(light->position, light->position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
            shadowTransforms.push_back(lightProj * glm::lookAt(light->position, light->position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

            glUniformMatrix4fv(program->getUniformLocation("viewProj[0]"), 6, GL_FALSE, glm::value_ptr(shadowTransforms[0]));
            glUniform1i(program->getUniformLocation("lightIndex"), light->shadowIndex);
            glUniform1f(program->getUniformLocation("far_plane"), 60.0f);
            glUniform3fv(program->getUniformLocation("lightPos"),1,glm::value_ptr(light->position));
            for (const auto& [name, obj] : scene->models) {

                if (!obj.visible)
                    continue;
                glUniformMatrix4fv(program->getUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(obj.matrix));
                obj.model->draw(program, obj.numInstance, false);
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    void ShadowManager::renderUI()
    {
#if defined(_MYDEBUG)
        ImGui::Begin("Shadow Debug");

        // Render Directional and Spot Light Shadow Maps
        if (ImGui::CollapsingHeader("Directional/Spot Shadow Maps"))
        {
            ImGui::SliderInt("ShadowMap Index", &currentShadowmaps, 0, maxShadowmaps);
            std::string label = "Shadow Map Layer " + std::to_string(currentShadowmaps);
            ImGui::Text("%s", label.c_str());
            // Render a layer of the 2D array shadow map
            ImGui::Image((void*)(intptr_t)shadowmap, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
        }

        // Render Point Light Shadow Maps
        if(ImGui::CollapsingHeader("Point Light Shadow Maps"))
        {
            ImGui::SliderInt("Cubemap Index", &currentPointShadowcubemaps, 0, maxPointShadowcubemaps);
            for (int face = 0; face < 6; ++face)
            {
                std::string label = "Cubemap " + std::to_string(currentPointShadowcubemaps) + " Face " + std::to_string(face);
                ImGui::Text("%s", label.c_str());
                int index = currentPointShadowcubemaps * 6 + face;
                ImGui::Image((void*)(intptr_t)debugcubemapFaceViews[index], ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
            }
        }
        ImGui::End();
#endif
    }
};