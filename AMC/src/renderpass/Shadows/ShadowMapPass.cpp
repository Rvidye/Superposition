#include<common.h>
#include "ShadowMapPass.h"

void ShadowMapPass::create(AMC::RenderContext& context) {
	m_programShadowMap = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\shadows\\shadowVS.vert"), RESOURCE_PATH("shaders\\shadows\\shadowGS.geom"), RESOURCE_PATH("shaders\\shadows\\shadowFS.frag") });
	m_programPointShadowMap = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\shadows\\shadowVS.vert"), RESOURCE_PATH("shaders\\shadows\\shadowPointGS.geom"), RESOURCE_PATH("shaders\\shadows\\shadowFS.frag") });
}

void ShadowMapPass::execute(const AMC::Scene* scene, AMC::RenderContext& context) {

	AMC::ShadowManager *sm = scene->lightManager->getShadowMapManager();
	sm->renderShadowMaps(m_programShadowMap, scene);
	sm->renderPointShadowMaps(m_programPointShadowMap, scene);
}