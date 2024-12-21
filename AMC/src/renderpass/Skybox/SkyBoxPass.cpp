#include<common.h>
#include<TextureManager.h>
#include "SkyBoxPass.h"

void SkyBoxPass::create(AMC::RenderContext& context) {

	m_ProgramSkybox = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\SkyBox\\SkyBox.vert"), RESOURCE_PATH("shaders\\SkyBox\\SkyBox.frag") });

	//testTexture = AMC::TextureManager::LoadKTX2Texture(RESOURCE_PATH("textures\\skybox\\skybox.ktx2"));
	//glTextureParameteri(testTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//glTextureParameteri(testTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//glTextureParameteri(testTexture, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	//glTextureParameteri(testTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//glTextureParameteri(testTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glGenerateTextureMipmap(testTexture);
	//glBindTextureUnit(10, 0);

	glCreateFramebuffers(1, &context.fboPostDeferred);
	glNamedFramebufferTexture(context.fboPostDeferred, GL_COLOR_ATTACHMENT0, context.textureDeferredResult, 0);
	glNamedFramebufferTexture(context.fboPostDeferred, GL_DEPTH_ATTACHMENT, context.textureGBuffer[4], 0);
	GLenum status = glCheckNamedFramebufferStatus(context.fboPostDeferred, GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		LOG_ERROR(L"PostDeferred framebuffer is not complete! %d", status);
	}
}

void SkyBoxPass::execute(AMC::Scene* scene, AMC::RenderContext& context) {

	glBindFramebuffer(GL_FRAMEBUFFER, context.fboPostDeferred);
	glDepthMask(GL_FALSE);
	m_ProgramSkybox->use();
	glBindTextureUnit(0, context.textureAtmosphere);
	glBindVertexArray(context.emptyVAO);
	glDrawArrays(GL_TRIANGLES,0,36);
	glDepthMask(GL_TRUE);
}

const char* SkyBoxPass::getName() const
{
	return "Skybox Pass";
}

void SkyBoxPass::renderUI()
{
#ifdef _MYDEBUG
#endif
}
