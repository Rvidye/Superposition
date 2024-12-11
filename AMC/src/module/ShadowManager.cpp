#pragma once

#include<ShadowManager.h>

static const int SHADOWMAP_SIZE = 2048;

namespace AMC {

    ShadowManager::ShadowManager(int maxDirSpot, int maxPoint) : maxShadowmaps(maxDirSpot), maxPointShadowcubemaps(maxPoint)
    {
        // Create 2D array for directional/spot shadows
        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &shadowmap);
        glTextureStorage3D(shadowmap, 1, GL_DEPTH_COMPONENT32F, SHADOWMAP_SIZE, SHADOWMAP_SIZE, maxShadowmaps);
        glTextureParameteri(shadowmap, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(shadowmap, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(shadowmap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(shadowmap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(shadowmap, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTextureParameteri(shadowmap, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

        // Create cube map array for point lights
        glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, &pointShadowCubemap);
        glTextureStorage3D(pointShadowCubemap, 1, GL_DEPTH_COMPONENT32F, SHADOWMAP_SIZE, SHADOWMAP_SIZE, 6 * maxPointShadowcubemaps);
        glTextureParameteri(pointShadowCubemap, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(pointShadowCubemap, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(pointShadowCubemap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(pointShadowCubemap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(pointShadowCubemap, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureParameteri(pointShadowCubemap, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTextureParameteri(pointShadowCubemap, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    }

    void ShadowManager::createShadowMapForLight(Light& light)
    {
        int idx = -1;
        if (light.type == LIGHT_TYPE_DIRECTIONAL || light.type == LIGHT_TYPE_SPOT) {
            if (currentShadowmaps < maxShadowmaps) {
                idx = currentShadowmaps++;
            }
        }
        else if (light.type == LIGHT_TYPE_POINT) {
            if (currentPointShadowcubemaps < maxPointShadowcubemaps) {
                idx = maxPointShadowcubemaps++;
            }
        }

        if (idx == -1) {
            LOG_WARNING(L"No available shadow map slots for this light.\n");
            light.shadows = false;
            return;
        }

        light.shadowIndex = idx;
    }

    void ShadowManager::removeShadowMapForLight(Light& light)
    {
        // Ideally should be a better mechanism for this but for now its okay.
        light.shadowIndex = -1;
    }

    void ShadowManager::renderShadowMaps(const Scene* scene)
    {
    }

    void ShadowManager::renderPointShadowMaps(const Scene* scene)
    {
    }



};