#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>

class Volumetric : public AMC::RenderPass {

public:
	void create(AMC::RenderContext& context) override;
	void execute(AMC::Scene* scene, AMC::RenderContext& context) override;
	const char* getName() const override;
	void renderUI() override;
	AMC::ShaderProgram* m_programVolumetricLighting;
	AMC::ShaderProgram* m_programUpscale;

	GLuint textureResult = 0, textureVolumetricLighting = 0, textureDepth = 0;

	glm::vec3 Absorbance = glm::vec3(0.025f);
	int SampleCount = 5;
	float Scattering = 0.758f;
	float MaxDist = 50.0f;
	float Strength = 0.1f;
};
