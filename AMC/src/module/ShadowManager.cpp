#pragma once

#include<ShadowManager.h>
#include<ShaderProgram.h>
#include<Scene.h>

static const int SHADOWMAP_SIZE = 512;

namespace AMC {

    Shadow::Shadow() {
        gpuShadow.ProjViewMatrices[0] = glm::mat4(1.0f);
        gpuShadow.ProjViewMatrices[1] = glm::mat4(1.0f);
        gpuShadow.ProjViewMatrices[2] = glm::mat4(1.0f);
        gpuShadow.ProjViewMatrices[3] = glm::mat4(1.0f);
        gpuShadow.ProjViewMatrices[4] = glm::mat4(1.0f);
        gpuShadow.ProjViewMatrices[5] = glm::mat4(1.0f);
        gpuShadow.Position = glm::vec3(0.0f);
        gpuShadow.NearPlane = 0.15f;
        gpuShadow.FarPlane = 60.0f;
        gpuShadow.LightIndex = -1;
        gpuShadow.NearestSampler = 0;
        gpuShadow.ShadowSampler = 0;
        fbo = 0;
        texture = 0;
    }

    Shadow::~Shadow()
    {
        // Make texture handles non-resident
//        glMakeTextureHandleNonResidentARB(gpuShadow.NearestSampler);
//        glMakeTextureHandleNonResidentARB(gpuShadow.ShadowSampler);
//#if defined(_MYDEBUG)
//        // Delete debug texture views
//        if (!debugcubemapFaceViews.empty()) {
//            glDeleteTextures(static_cast<GLsizei>(debugcubemapFaceViews.size()), debugcubemapFaceViews.data());
//        }
//#endif
//        // Delete framebuffer and texture
//        glDeleteFramebuffers(1, &fbo);
//        glDeleteTextures(1, &texture);
    }

    void Shadow::UpdateViewMatrices() {
       gpuShadow.ProjViewMatrices[0] = projection * glm::lookAt(gpuShadow.Position, gpuShadow.Position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
       gpuShadow.ProjViewMatrices[1] = projection * glm::lookAt(gpuShadow.Position, gpuShadow.Position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
       gpuShadow.ProjViewMatrices[2] = projection * glm::lookAt(gpuShadow.Position, gpuShadow.Position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
       gpuShadow.ProjViewMatrices[3] = projection * glm::lookAt(gpuShadow.Position, gpuShadow.Position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
       gpuShadow.ProjViewMatrices[4] = projection * glm::lookAt(gpuShadow.Position, gpuShadow.Position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
       gpuShadow.ProjViewMatrices[5] = projection * glm::lookAt(gpuShadow.Position, gpuShadow.Position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    }

    /*
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
    */

    ShadowManager::ShadowManager()
    {
        shadows.reserve(MAX_SHADOWS);
        Shadow hack[MAX_SHADOWS];
        glCreateBuffers(1, &shadowsUBO);
        glNamedBufferData(shadowsUBO, sizeof(GpuShadow) * MAX_SHADOWS + sizeof(int), nullptr, GL_DYNAMIC_DRAW);
        for (size_t i = 0; i < MAX_SHADOWS; ++i) {
            glNamedBufferSubData(shadowsUBO, sizeof(GpuShadow) * i, sizeof(GpuShadow), &hack[i].gpuShadow);
        }
        int count = 0;
        glNamedBufferSubData(shadowsUBO, sizeof(GpuShadow) * MAX_SHADOWS, sizeof(int), &count);
        glBindBufferBase(GL_UNIFORM_BUFFER, 2, shadowsUBO);
    }

    ShadowManager::~ShadowManager() {
        glDeleteBuffers(1, &shadowsUBO);
    }

    bool ShadowManager::AddShadow(int lightIndex, Light& light)
    {
        if (shadows.size() >= MAX_SHADOWS) {
            LOG_ERROR(L"Max Shadows Limit Reached\n");
            return false;
        }
        Shadow newShadow;
        newShadow.gpuShadow.Position = glm::vec3(0.0f);
        newShadow.gpuShadow.NearPlane = 0.15f;
        newShadow.gpuShadow.FarPlane = 60.0f;
        newShadow.gpuShadow.LightIndex = -1;

        newShadow.projection = glm::perspective(glm::radians(90.0f), 1.0f, newShadow.gpuShadow.NearPlane, newShadow.gpuShadow.FarPlane);
        newShadow.gpuShadow.LightIndex = lightIndex;
        newShadow.gpuShadow.Position = light.gpuLight.position;
        newShadow.UpdateViewMatrices();
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &newShadow.texture);
        //glTextureParameteri(newShadow.texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        //glTextureParameteri(newShadow.texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        //glTextureParameteri(newShadow.texture, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureStorage2D(newShadow.texture, 1, GL_DEPTH_COMPONENT16, SHADOWMAP_SIZE, SHADOWMAP_SIZE);

        glCreateFramebuffers(1, &newShadow.fbo);
        glNamedFramebufferTexture(newShadow.fbo, GL_DEPTH_ATTACHMENT, newShadow.texture, 0);
        glNamedFramebufferDrawBuffer(newShadow.fbo, GL_NONE);
        glNamedFramebufferReadBuffer(newShadow.fbo, GL_NONE);
        GLenum status = glCheckNamedFramebufferStatus(newShadow.fbo, GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            LOG_ERROR(L"Point Shadowmap FBO is not complete\n");
        }

        GLuint nearestSampler, shadowSampler;
        glCreateSamplers(1, &shadowSampler);
        glSamplerParameteri(shadowSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glSamplerParameteri(shadowSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glSamplerParameteri(shadowSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(shadowSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(shadowSampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(shadowSampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glSamplerParameteri(shadowSampler, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

        glCreateSamplers(1, &nearestSampler);
        glSamplerParameteri(nearestSampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glSamplerParameteri(nearestSampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glSamplerParameteri(nearestSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(nearestSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(nearestSampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(nearestSampler, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        glSamplerParameteri(nearestSampler, GL_TEXTURE_COMPARE_FUNC, GL_ALWAYS);

        newShadow.gpuShadow.NearestSampler = glGetTextureSamplerHandleARB(newShadow.texture, nearestSampler);
        glMakeTextureHandleResidentARB(newShadow.gpuShadow.NearestSampler);

        newShadow.gpuShadow.ShadowSampler = glGetTextureSamplerHandleARB(newShadow.texture, shadowSampler);
        glMakeTextureHandleResidentARB(newShadow.gpuShadow.ShadowSampler);
#if defined(_MYDEBUG)
        if (newShadow.debugcubemapFaceViews.empty())
        {
            newShadow.debugcubemapFaceViews.resize(6);
            glGenTextures(6, newShadow.debugcubemapFaceViews.data());
            for (int face = 0; face < 6; ++face)
            {
                glTextureView(newShadow.debugcubemapFaceViews[face], GL_TEXTURE_2D, newShadow.texture, GL_DEPTH_COMPONENT16, 0, 1, face, 1);
            }
        }
#endif
        shadows.push_back(newShadow);
        UpdateShadowUBO();
        return true;
    }

    void ShadowManager::DeleteShadow(int shadowIndex)
    {
        if (shadowIndex < 0 || shadowIndex >= static_cast<int>(shadows.size())) {
            LOG_ERROR(L"Shadow %d index does not exist\n", shadowIndex);
            return;
        }
        // Swap with the last shadow to maintain a contiguous array
        shadows[shadowIndex] = shadows.back();
        shadows.pop_back();
        UpdateShadowUBO();
    }

    void ShadowManager::UpdateShadows(const std::vector<Light>& lights)
    {
        for (size_t i = 0; i < shadows.size(); ++i) {
            int lightIndex = shadows[i].gpuShadow.LightIndex;
            if (lightIndex >= 0 && lightIndex < static_cast<int>(lights.size())) {
                shadows[i].gpuShadow.Position = lights[lightIndex].gpuLight.position;
                shadows[i].needsUpdate = true;
                shadows[i].UpdateViewMatrices();
            }
        }
        UpdateShadowUBO();
    }

    void ShadowManager::UpdateShadowUBO()
    {
        for (size_t i = 0; i < shadows.size(); ++i) {
            glNamedBufferSubData(shadowsUBO, sizeof(GpuShadow) * i, sizeof(GpuShadow), &shadows[i].gpuShadow);
        }
        // Update shadow count
        int shadowCount = static_cast<int>(shadows.size());
        glNamedBufferSubData(shadowsUBO, sizeof(GpuShadow) * MAX_SHADOWS, sizeof(int), &shadowCount);
    }

    void ShadowManager::RenderShadowMaps(ShaderProgram* program, const Scene* scene)
    {
        for (int shadowIndex = 0; shadowIndex < static_cast<int>(shadows.size()); ++shadowIndex) {
            Shadow& shadow = shadows[shadowIndex];
            //if (!shadow.needsUpdate) continue;
            float depthClear = 1.0f;
            glClearNamedFramebufferfv(shadow.fbo, GL_DEPTH, 0, &depthClear);
            glBindFramebuffer(GL_FRAMEBUFFER, shadow.fbo);
            glDisable(GL_CULL_FACE);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
            glDisable(GL_CONSERVATIVE_RASTERIZATION_NV);
            glEnable(GL_DEPTH_TEST);
            glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glCullFace(GL_BACK);
            glDepthFunc(GL_LESS);
            glViewport(0, 0, SHADOWMAP_SIZE, SHADOWMAP_SIZE);
            program->use();
            glUniform1i(program->getUniformLocation("shadowIndex"), shadowIndex);
            for (const auto& [name, obj] : scene->models) {
                if (!obj.visible)
                    continue;
                glUniformMatrix4fv(program->getUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(obj.matrix));
                obj.model->draw(program, obj.numInstance, false);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            //shadow.needsUpdate = false;
        }
    }

    void ShadowManager::UpdateShadowLightIndex(int shadowIndex, int newLightIndex)
    {
        if (shadowIndex < 0 || shadowIndex >= static_cast<int>(shadows.size())) {
            LOG_ERROR(L"Shadow index out of range.");
            return;
        }
        shadows[shadowIndex].gpuShadow.LightIndex = newLightIndex;
        shadows[shadowIndex].needsUpdate = true;
        shadows[shadowIndex].UpdateViewMatrices();
        UpdateShadowUBO();
    }

    //void ShadowManager::renderShadowMaps(ShaderProgram* program, const Scene* scene)
    //{
    //    // Compute all directional and spot light matrices and store in buffer
    //    auto lights = scene->lightManager->getShadowCastingLights();
    //    std::vector<glm::mat4> dirSpotMatrices;
    //    dirSpotMatrices.reserve(3);

    //    const AABB& sceneAABB = scene->sceneAABB;

    //    for (auto* light : lights) {
    //        if (!light->shadows) continue;
    //        if (light->type == LIGHT_TYPE_POINT) continue;
    //        glm::mat4 lightView = glm::lookAt(light->position, light->position + light->direction, glm::vec3(0, 1, 0));

    //        if (light->type == LIGHT_TYPE_DIRECTIONAL) {

    //            // too complex 
    //            glm::vec3 vertices[8] = {
    //                glm::vec3(sceneAABB.mMin.x, sceneAABB.mMin.y, sceneAABB.mMin.z),
    //                glm::vec3(sceneAABB.mMax.x, sceneAABB.mMin.y, sceneAABB.mMin.z),
    //                glm::vec3(sceneAABB.mMin.x, sceneAABB.mMax.y, sceneAABB.mMin.z),
    //                glm::vec3(sceneAABB.mMax.x, sceneAABB.mMax.y, sceneAABB.mMin.z),
    //                glm::vec3(sceneAABB.mMin.x, sceneAABB.mMin.y, sceneAABB.mMax.z),
    //                glm::vec3(sceneAABB.mMax.x, sceneAABB.mMin.y, sceneAABB.mMax.z),
    //                glm::vec3(sceneAABB.mMin.x, sceneAABB.mMax.y, sceneAABB.mMax.z),
    //                glm::vec3(sceneAABB.mMax.x, sceneAABB.mMax.y, sceneAABB.mMax.z),
    //            };

    //            glm::vec3 lightSpaceMin(FLT_MAX);
    //            glm::vec3 lightSpaceMax(-FLT_MAX);

    //            for (const auto& vertex : vertices) {
    //                glm::vec3 lightSpaceVertex = glm::vec3(lightView * glm::vec4(vertex, 1.0f));
    //                lightSpaceMin = glm::min(lightSpaceMin, lightSpaceVertex);
    //                lightSpaceMax = glm::max(lightSpaceMax, lightSpaceVertex);
    //            }

    //            // not perfect but will do job for now
    //            //glm::vec3 lightDir = glm::normalize(light->direction);
    //            //float minZ = glm::dot(sceneAABB.mMin, lightDir);
    //            //float maxZ = glm::dot(sceneAABB.mMax, lightDir);
    //            //float left = sceneAABB.mMin.x;
    //            //float right = sceneAABB.mMax.x;
    //            //float bottom = sceneAABB.mMin.y;
    //            //float top = sceneAABB.mMax.y;

    //            glm::mat4 lightProj = glm::ortho(lightSpaceMin.x, lightSpaceMax.x, lightSpaceMin.y, lightSpaceMax.y, -lightSpaceMax.z, -lightSpaceMin.z);
    //            dirSpotMatrices.push_back(lightProj * lightView);
    //        }
    //        else {
    //            glm::mat4 lightProj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    //            dirSpotMatrices.push_back(lightProj * lightView);
    //        }
    //    }

    //    float depthClear = 1.0f;
    //    glClearNamedFramebufferfv(shadowmapFBO, GL_DEPTH, 0, &depthClear);
    //    glBindFramebuffer(GL_FRAMEBUFFER, shadowmapFBO);
    //    glViewport(0, 0, SHADOWMAP_SIZE, SHADOWMAP_SIZE);
    //    program->use();
    //    glUniformMatrix4fv(program->getUniformLocation("viewProj[0]"), 3, GL_FALSE, glm::value_ptr(dirSpotMatrices[0]));
    //    glUniform1i(program->getUniformLocation("numShadows"), (GLint)dirSpotMatrices.size());
    //    for (const auto& [name, obj] : scene->models) {

    //        if (!obj.visible)
    //            continue;
    //        glUniformMatrix4fv(program->getUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(obj.matrix));
    //        obj.model->draw(program,obj.numInstance,false);
    //    }
    //}

    //void ShadowManager::renderPointShadowMaps(ShaderProgram* program, const Scene* scene)
    //{
    //    float depthClear = 1.0f;
    //    glClearNamedFramebufferfv(pointShadowmapFBO, GL_DEPTH, 0, &depthClear);
    //    glBindFramebuffer(GL_FRAMEBUFFER, pointShadowmapFBO);
    //    glViewport(0, 0, SHADOWMAP_SIZE, SHADOWMAP_SIZE);
    //    program->use();
    //    auto lights = scene->lightManager->getShadowCastingLights();

    //    for (auto* light : lights) {
    //        if (!light->active) continue;
    //        if (!light->shadows) continue;
    //        if (light->type != LIGHT_TYPE_POINT) continue;

    //        glm::mat4 lightProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.15f, 60.0f);

    //        std::vector<glm::mat4> shadowTransforms;
    //        shadowTransforms.push_back(lightProj * glm::lookAt(light->position, light->position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
    //        shadowTransforms.push_back(lightProj * glm::lookAt(light->position, light->position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
    //        shadowTransforms.push_back(lightProj * glm::lookAt(light->position, light->position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
    //        shadowTransforms.push_back(lightProj * glm::lookAt(light->position, light->position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
    //        shadowTransforms.push_back(lightProj * glm::lookAt(light->position, light->position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
    //        shadowTransforms.push_back(lightProj * glm::lookAt(light->position, light->position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

    //        glUniformMatrix4fv(program->getUniformLocation("viewProj[0]"), 6, GL_FALSE, glm::value_ptr(shadowTransforms[0]));
    //        glUniform1i(program->getUniformLocation("lightIndex"), light->shadowIndex);
    //        glUniform1f(program->getUniformLocation("far_plane"), 60.0f);
    //        glUniform3fv(program->getUniformLocation("lightPos"),1,glm::value_ptr(light->position));
    //        for (const auto& [name, obj] : scene->models) {

    //            if (!obj.visible)
    //                continue;
    //            glUniformMatrix4fv(program->getUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(obj.matrix));
    //            obj.model->draw(program, obj.numInstance, false);
    //        }
    //    }
    //    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //}

    void ShadowManager::renderUI()
    {
#if defined(_MYDEBUG)
        if (shadows.empty()) {
            ImGui::Text("No shadows to display.");
            return;
        }

        ImGui::Begin("Shadow Debug");
        static int selectedShadow = 0; // Currently selected shadow index
        if (ImGui::BeginCombo("Select Shadow", ("Shadow " + std::to_string(selectedShadow)).c_str())) {
            for (int i = 0; i < static_cast<int>(shadows.size()); ++i) {
                bool isSelected = (selectedShadow == i);
                if (ImGui::Selectable(("Shadow " + std::to_string(i)).c_str(), isSelected)) {
                    selectedShadow = i;
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        Shadow& currentShadow = shadows[selectedShadow];
        const char* faceNames[] = { "Positive X", "Negative X", "Positive Y", "Negative Y", "Positive Z", "Negative Z" };
        for (int face = 0; face < 6; ++face)
        {
            std::string label = "Cubemap Face " + std::string(faceNames[face]);
            ImGui::Text("%s", label.c_str());
            ImGui::Image((void*)(intptr_t)currentShadow.debugcubemapFaceViews[face], ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
        }
        ImGui::End();
#endif
    }
    void ShadowManager::BindUBO()
    {
        glBindBufferBase(GL_UNIFORM_BUFFER, 2, shadowsUBO);
    }
};