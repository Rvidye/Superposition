#include<common.h>
#include"DeferredLightPass.h"

void DeferredPass::create(AMC::RenderContext& context)
{
	m_ProgramDeferredLighting = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\ToScreen\\FullScreen.vert"), RESOURCE_PATH("shaders\\DeferredLighting\\DeferredLighting.frag") });

	glCreateFramebuffers(1, &m_FBO);
	
	glCreateTextures(GL_TEXTURE_2D, 1, &m_TextureResult);
	glTextureParameteri(m_TextureResult, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(m_TextureResult, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(m_TextureResult, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_TextureResult, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(m_TextureResult, 1, GL_RGBA16F, context.width, context.width);
	glNamedFramebufferTexture(m_FBO, GL_COLOR_ATTACHMENT0, m_TextureResult, 0);

	GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glNamedFramebufferDrawBuffers(m_FBO, 1, drawBuffers);

	GLenum status = glCheckNamedFramebufferStatus(m_FBO, GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		LOG_ERROR(L"Deferred Lighting framebuffer is not complete! %d", status);
	}

	context.textureDeferredResult = m_TextureResult;
}

void DeferredPass::execute(AMC::Scene* scene, AMC::RenderContext& context)
{
	if (!context.IsDeferredLighting) {
		return;
	}

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	glViewport(0, 0, context.width, context.height);
	glm::vec4 clearcolor = glm::vec4(0.0, 0.5, 0.5f, 1.0f);
	glClearNamedFramebufferfv(m_FBO, GL_COLOR, 0, glm::value_ptr(clearcolor));

	m_ProgramDeferredLighting->use();
	glUniform1i(m_ProgramDeferredLighting->getUniformLocation("IsVXGI"), context.IsVGXI);
	if(context.IsSSAO)
		glBindTextureUnit(5, context.textureSSAOResult);
	else
		glBindTextureUnit(5, 0);
	if (context.IsVGXI)
		glBindTextureUnit(6, context.textureVXGIResult);
	else
		glBindTextureUnit(6, 0);	
	scene->lightManager->BindUBO();
	scene->lightManager->GetShadowManager()->BindUBO();
	glBindVertexArray(context.emptyVAO);
	glDrawArrays(GL_TRIANGLES,0,3);
	glBindVertexArray(0);
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

const char* DeferredPass::getName() const
{
	return "Deferred Pass";
}

void DeferredPass::renderUI()
{
#ifdef _MYDEBUG
	ImGui::Begin("GBuffer Debug");
	ImGui::Text("Albedo");
	ImGui::Image((void*)(intptr_t)m_TextureResult, ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
	ImGui::End();
#endif
}
