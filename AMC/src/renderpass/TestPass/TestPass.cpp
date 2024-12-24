#include<common.h>
#include "TestPass.h"

void TestPass::create(AMC::RenderContext& context){

	m_programTexturedDraw = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\model\\model.vert"), RESOURCE_PATH("shaders\\model\\model.frag") });
}

void TestPass::execute(AMC::Scene* scene, AMC::RenderContext& context){

	AMC::Renderer::resetFBO();
	AMC::ShadowManager* sm = scene->lightManager->GetShadowManager();
	
	m_programTexturedDraw->use();

	for (const auto& [name, obj] : scene->models) {

		if (!obj.visible)
			continue;

		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(obj.matrix));
		//scene->lightManager->bindUBO(m_programTexturedDraw->getProgramObject());
		obj.model->draw(m_programTexturedDraw);
	}
}

const char* TestPass::getName() const
{
	return "nullptr";
}

void TestPass::renderUI()
{
#ifdef _MYDEBUG
#endif
}
