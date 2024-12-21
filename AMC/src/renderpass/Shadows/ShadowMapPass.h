#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>
#include<ShadowManager.h>

class ShadowMapPass : public AMC::RenderPass {

public:
	void create(AMC::RenderContext& context) override;
	void execute(AMC::Scene* scene, AMC::RenderContext& context) override;
	const char* getName() const override;
	void renderUI() override;
	AMC::ShaderProgram* m_programShadowMap;
	AMC::ShaderProgram* m_programPointShadowMap;
#ifdef _MYDEBUG
	AMC::ShadowManager* sm;
#endif
};
