#include<common.h>
#include"pbrPass.h"

void PBRPass::create() {

    m_programTexturedDraw = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\PBR\\pbr.vert"), RESOURCE_PATH("shaders\\PBR\\pbr.frag") });
}

void PBRPass::execute(const AMC::Scene* scene) {

    m_programTexturedDraw->use();

    for (const auto& [name, obj] : scene->models) {

        if (!obj.visible)
            continue;

        glUniformMatrix4fv(m_programTexturedDraw->getUniformLocation("mMat"), 1, GL_FALSE, glm::value_ptr(obj.matrix));
        glUniformMatrix4fv(m_programTexturedDraw->getUniformLocation("vMat"), 1, GL_FALSE, glm::value_ptr(AMC::currentCamera->getViewMatrix()));
        glUniformMatrix4fv(m_programTexturedDraw->getUniformLocation("pMat"), 1, GL_FALSE, glm::value_ptr(AMC::currentCamera->getProjectionMatrix()));
       



        obj.model->draw(m_programTexturedDraw);
    }
}
