#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>

class ConeTracer : public AMC::RenderPass {

public:
	void create(AMC::RenderContext& context) override;
	void execute(AMC::Scene* scene, AMC::RenderContext& context) override;
	const char* getName() const override;
	void renderUI() override;
	AMC::ShaderProgram* m_programConeTrace;
	GLuint textureResultConeTrace = 0;
	float NormalRayOffset = 1.0f;
	int MaxSamples = 5;
	float GIBoost = 1.3f;
	float GISkyBoxBoost = 1.0f / 1.3f;
	float StepMultiplier = 0.50f;
};