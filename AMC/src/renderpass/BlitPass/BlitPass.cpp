#include<common.h>
#include"BlitPass.h"

void BlitPass::create(AMC::RenderContext& context)
{
	m_ProgramBlit = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\ToScreen\\FullScreen.vert"), RESOURCE_PATH("shaders\\ToScreen\\FullScreen.frag") });
}

void BlitPass::execute(const AMC::Scene* scene, AMC::RenderContext& context)
{
	AMC::Renderer::resetFBO();
	m_ProgramBlit->use();
	glBindTextureUnit(10, context.textureDeferredResult);
	glBindVertexArray(context.emptyVAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
}
