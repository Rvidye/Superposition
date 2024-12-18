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

void DeferredPass::execute(const AMC::Scene* scene, AMC::RenderContext& context)
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	glViewport(0, 0, context.width, context.height);
	glm::vec4 clearcolor = glm::vec4(0.0, 0.5, 0.5f, 1.0f);
	glClearNamedFramebufferfv(m_FBO, GL_COLOR, 0, glm::value_ptr(clearcolor));

	m_ProgramDeferredLighting->use();

	glBindTextureUnit(0, context.textureGBuffer[0]);
	glBindTextureUnit(1, context.textureGBuffer[1]);
	glBindTextureUnit(2, context.textureGBuffer[2]);
	glBindTextureUnit(3, context.textureGBuffer[3]);
	glBindTextureUnit(4, context.textureGBuffer[4]);
	glBindTextureUnit(5, context.textureSSAOResult);
	scene->lightManager->bindUBO();
	glBindVertexArray(context.emptyVAO);
	glDrawArrays(GL_TRIANGLES,0,3);
	glBindVertexArray(0);
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
