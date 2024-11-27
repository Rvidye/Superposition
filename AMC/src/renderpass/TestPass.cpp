#include<common.h>
#include "TestPass.h"

void TestPass::create(){

	m_programTexturedDraw = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\model\\model.vert"), RESOURCE_PATH("shaders\\model\\model.frag") });
}

void TestPass::execute(const AMC::Scene* scene){

	m_programTexturedDraw->use();

	for (const auto& [name, obj] : scene->models) {

		if (!obj.visible)
			continue;

		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(AMC::currentCamera->getProjectionMatrix() * AMC::currentCamera->getViewMatrix() * obj.matrix));
		obj.model->draw(m_programTexturedDraw);
	}
}
