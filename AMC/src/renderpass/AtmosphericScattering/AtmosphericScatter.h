#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>

class AtmosphericScatterer : public AMC::RenderPass {

public:
	void create(AMC::RenderContext& context) override;
	void execute(AMC::Scene* scene, AMC::RenderContext& context) override;
	const char* getName() const override;
	void renderUI() override;
	AMC::ShaderProgram* m_ProgramAtmosphericScatter;
	GLuint textureAtmosphericResult;
	bool enableAtmosphericScattering = true;
	int ISteps = 40;
	int JSteps = 8;
	float LightIntensity = 15.0f;
	float Azimuth = 0.0f;
	float Elevation = 0.0f;
};