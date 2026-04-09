#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>

class HairSimulationPass : public AMC::RenderPass {
public:
	void create(AMC::RenderContext& context) override;
	void execute(AMC::Scene* scene, AMC::RenderContext& context) override;
	const char* getName() const override;
	void renderUI() override;

private:
	AMC::ShaderProgram* m_simulateProgram = nullptr;
	AMC::ShaderProgram* m_rebuildProgram = nullptr;
	float m_timeAccumulator = 0.0f;
};
