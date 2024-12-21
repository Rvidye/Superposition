#include<common.h>
#include"BlitPass.h"

void BlitPass::create(AMC::RenderContext& context)
{
	m_ProgramBlit = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\ToScreen\\FullScreen.vert"), RESOURCE_PATH("shaders\\ToScreen\\FullScreen.frag") });
}

void BlitPass::execute(AMC::Scene* scene, AMC::RenderContext& context)
{
	AMC::Renderer::resetFBO();
	m_ProgramBlit->use();
	glBindTextureUnit(10, context.textureTonemapResult);
	glBindVertexArray(context.emptyVAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
}

const char* BlitPass::getName() const
{
	return "##";
}

void BlitPass::renderUI()
{
#ifdef _MYDEBUG
#endif
}
