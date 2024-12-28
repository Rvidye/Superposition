#include<common.h>
#include "VolumetricLighting.h"

void Volumetric::create(AMC::RenderContext& context) {
	m_programVolumetricLighting = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\VolumetricLight\\VolumetricLight.comp")});
    m_programUpscale = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\VolumetricLight\\Upscale.comp") });

    glm::ivec2 RenderResolution = glm::ivec2(context.width * 0.6f, context.height * 0.6f); // replace later with render resolution

    glCreateTextures(GL_TEXTURE_2D, 1, &textureResult);
    glTextureParameteri(textureResult, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(textureResult, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(textureResult, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(textureResult, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureStorage2D(textureResult, 1, GL_RGBA16F, context.width, context.height);

    glCreateTextures(GL_TEXTURE_2D, 1, &textureVolumetricLighting);
    glTextureParameteri(textureVolumetricLighting, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(textureVolumetricLighting, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(textureVolumetricLighting, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(textureVolumetricLighting, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureStorage2D(textureVolumetricLighting, 1, GL_RGBA16F, RenderResolution.x, RenderResolution.y);

    glCreateTextures(GL_TEXTURE_2D, 1, &textureDepth);
    glTextureParameteri(textureDepth, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(textureDepth, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(textureDepth, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(textureDepth, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureStorage2D(textureDepth, 1, GL_R32F, RenderResolution.x, RenderResolution.y);

    context.textureVolumetricResult = textureResult;
}

void Volumetric::execute(AMC::Scene* scene, AMC::RenderContext& context) {

    glBindImageTexture(0, textureVolumetricLighting, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
    glBindImageTexture(1, textureDepth, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
	m_programVolumetricLighting->use();
    glUniform3fv(m_programVolumetricLighting->getUniformLocation("Absorbance"), 1, glm::value_ptr(Absorbance));
    glUniform1i(m_programVolumetricLighting->getUniformLocation("SampleCount"), SampleCount);
    glUniform1f(m_programVolumetricLighting->getUniformLocation("Scattering"), Scattering);
    glUniform1f(m_programVolumetricLighting->getUniformLocation("MaxDist"), MaxDist);
    glUniform1f(m_programVolumetricLighting->getUniformLocation("Strength"), Strength);
    GLuint workGroupSizeX = (context.width + 8 - 1) / 8;
    GLuint workGroupSizeY = (context.height + 8 - 1) / 8;
    glDispatchCompute(workGroupSizeX, workGroupSizeY, 1);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

    glBindImageTexture(0, textureResult, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
    glBindTextureUnit(0, textureVolumetricLighting);
    glBindTextureUnit(1, textureDepth);
    m_programUpscale->use();
    glDispatchCompute(workGroupSizeX, workGroupSizeY, 1);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
}

const char* Volumetric::getName() const
{
	return "nullptr";
}

void Volumetric::renderUI()
{
#ifdef _MYDEBUG

    ImGui::SliderInt("MaxSamples", &SampleCount, 1, 30);
    ImGui::SliderFloat("Scattering", &Scattering, 0.0f, 1.0f);
    ImGui::SliderFloat("Stength", &Strength, 0.0f, 1.0f);
    ImGui::InputFloat3("Absorbance", &Absorbance.x);
    ImGui::Begin("Volumetric Textures");
    ImGui::Text("Volumetric Light Texture");
    ImGui::Image((void*)(intptr_t)textureVolumetricLighting, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
    ImGui::Text("Depth Texture");
    ImGui::Image((void*)(intptr_t)textureDepth, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
    ImGui::Text("Result Texture");
    ImGui::Image((void*)(intptr_t)textureResult, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
    ImGui::End();
#endif
}