#include<common.h>
#include "ShadowMapPass.h"
#include <RenderExtractor.h>

void ShadowMapPass::create(AMC::RenderContext& context) {
	m_programPointShadowMap = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\shadows\\shadowVS.vert"), RESOURCE_PATH("shaders\\shadows\\shadowPointGS.geom"), RESOURCE_PATH("shaders\\shadows\\shadowFS.frag") });
	m_programShadowMesh = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\shadows\\shadow_indirect.task"), RESOURCE_PATH("shaders\\shadows\\shadow_indirect.mesh"), RESOURCE_PATH("shaders\\shadows\\shadow_indirect.frag") });
}

void ShadowMapPass::execute(AMC::Scene* scene, AMC::RenderContext& context) {

	if (!context.IsGenerateShadowMaps)
		return;

	// Skip raster shadow maps entirely if no consumer needs them
	if (!context.needsRasterShadows)
		return;

	// Skip if neither geometry nor lights are dirty
	if (!context.dirtyFlags.rasterShadowDirty)
		return;

	AMC::ShadowManager *sm = scene->lightManager->GetShadowManager();
	auto& extractor = AMC::RenderExtractor::Get();

	if (context.UseMeshShaders && m_programShadowMesh) {
		sm->RenderShadowMapsMesh(m_programShadowMesh, m_programPointShadowMap,
			scene, extractor, true);
	} else {
		sm->RenderShadowMaps(m_programPointShadowMap, scene);
	}

	#ifdef _MYDEBUG
	this->sm = sm;
	#endif
}

const char* ShadowMapPass::getName() const
{
	return "Shadowmap Pass";
}

void ShadowMapPass::renderUI()
{
#ifdef _MYDEBUG
	if (sm) sm->renderUI();
#endif
}
