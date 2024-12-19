#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>

class ShadowMapPass : public AMC::RenderPass {

public:
	void create(AMC::RenderContext& context) override;
	void execute(const AMC::Scene* scene, AMC::RenderContext& context) override;
	const char* getName() const override;
	void renderUI() override;
	AMC::ShaderProgram* m_programShadowMap;
	AMC::ShaderProgram* m_programPointShadowMap;
};
