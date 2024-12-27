#include<common.h>
#include"BlitPass.h"

void BlitPass::create(AMC::RenderContext& context)
{
	m_ProgramBlit = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\ToScreen\\FullScreen.vert"), RESOURCE_PATH("shaders\\ToScreen\\FullScreen.frag") });
}

void BlitPass::execute(AMC::Scene* scene, AMC::RenderContext& context)
{
	AMC::Renderer::resetFBO();
	glEnable(GL_BLEND);
	//glEnable(GL_SCISSOR_TEST);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
