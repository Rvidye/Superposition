#include<common.h>
#include "GBufferPass.h"

void GBufferPass::create()
{
	m_ProgramGBuffer = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\gbuffer\\gbuffer.vert"), RESOURCE_PATH("shaders\\gbuffer\\gbuffer.frag") });

	glCreateFramebuffers(1, &gbuffer);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_textureAlbedoAlpha);
    glTextureParameteri(m_textureAlbedoAlpha, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_textureAlbedoAlpha, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_textureAlbedoAlpha, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_textureAlbedoAlpha, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureStorage2D(m_textureAlbedoAlpha, 1, GL_RGBA8, 4096, 4096);
    glNamedFramebufferTexture(gbuffer, GL_COLOR_ATTACHMENT0, m_textureAlbedoAlpha, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_textureNormal);
    glTextureParameteri(m_textureNormal, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_textureNormal, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_textureNormal, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_textureNormal, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureStorage2D(m_textureNormal, 1, GL_RGB16F, 4096, 4096);
    glNamedFramebufferTexture(gbuffer, GL_COLOR_ATTACHMENT1, m_textureNormal, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_textureMetallicRoughness);
    glTextureParameteri(m_textureMetallicRoughness, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_textureMetallicRoughness, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_textureMetallicRoughness, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_textureMetallicRoughness, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureStorage2D(m_textureMetallicRoughness, 1, GL_RG8, 4096, 4096);
    glNamedFramebufferTexture(gbuffer, GL_COLOR_ATTACHMENT2, m_textureMetallicRoughness, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_textureEmissive);
    glTextureParameteri(m_textureEmissive, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_textureEmissive, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_textureEmissive, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_textureEmissive, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureStorage2D(m_textureEmissive, 1, GL_RGB16F, 4096, 4096);
    glNamedFramebufferTexture(gbuffer, GL_COLOR_ATTACHMENT3, m_textureEmissive, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_textureDepth);
    glTextureParameteri(m_textureDepth, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_textureDepth, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_textureDepth, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_textureDepth, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureStorage2D(m_textureDepth, 1, GL_DEPTH_COMPONENT32F, 4096, 4096);
    glNamedFramebufferTexture(gbuffer, GL_DEPTH_ATTACHMENT, m_textureDepth, 0);

    GLenum drawBuffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    glNamedFramebufferDrawBuffers(gbuffer, 4, drawBuffers);
    GLenum status = glCheckNamedFramebufferStatus(gbuffer, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR(L"GBuffer framebuffer is not complete! %d", status);
    }
}

void GBufferPass::execute(const AMC::Scene* scene)
{
    glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);
    glViewport(0, 0, 4096, 4096);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_ProgramGBuffer->use();
    for (const auto& [name, obj] : scene->models) {
        if (!obj.visible)
            continue;

        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(obj.matrix));
        scene->lightManager->bindUBO(m_ProgramGBuffer->getProgramObject());
        obj.model->draw(m_ProgramGBuffer);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GBufferPass::debugGBuffer()
{
#if defined(_MYDEBUG)
    ImGui::Begin("GBuffer Debug");
    ImGui::Text("Albedo");
    ImGui::Image((void*)(intptr_t)m_textureAlbedoAlpha, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
    ImGui::Text("Normal");
    ImGui::Image((void*)(intptr_t)m_textureNormal, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
    ImGui::Text("MetallicRough");
    ImGui::Image((void*)(intptr_t)m_textureMetallicRoughness, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
    ImGui::Text("Emissive");
    ImGui::Image((void*)(intptr_t)m_textureEmissive, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
    ImGui::Text("Depth");
    ImGui::Image((void*)(intptr_t)m_textureDepth, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
    ImGui::End();
#endif
}
