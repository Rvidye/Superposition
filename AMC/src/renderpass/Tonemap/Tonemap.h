#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>

class Tonemap : public AMC::RenderPass {

public:
	void create(AMC::RenderContext& context) override;
	void execute(const AMC::Scene* scene, AMC::RenderContext& context) override;
	const char* getName() const override;
	void renderUI() override;
	AMC::ShaderProgram* m_ProgramTonemap;

	GLuint textureTonemapResult;

	float Exposure = 0.45f;
	float Saturation = 1.06f;
	float Linear = 0.18f;
	float Peak = 1.0f;
	float Compression = 0.10f;
	bool IsAgXTonemaping = true;

};