#include<common.h>
#include "DebugDrawPass.h"

void DebugDrawPass::create(AMC::RenderContext& context) {
}

void DebugDrawPass::execute(AMC::Scene* scene, AMC::RenderContext& context) {

	//Just render debug object like lights/spline into gbuffer
	scene->renderDebug();
	if (context.IsHairDebugWireframe) {
		scene->renderHairDebug(context);
	}
}

const char* DebugDrawPass::getName() const
{
	return "DebugDraw";
}

void DebugDrawPass::renderUI()
{
#ifdef _MYDEBUG
#endif
}
