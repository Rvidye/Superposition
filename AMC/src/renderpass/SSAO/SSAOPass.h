#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>

class SSAO : public AMC::RenderPass {

public:
	void create(AMC::RenderContext& context) override;
	void execute(AMC::Scene* scene, AMC::RenderContext& context) override;
	const char* getName() const override;
	void renderUI() override;
	AMC::ShaderProgram* m_ProgramCompuetSSAO;
	GLuint m_textureResult;
	int SampleCount = 10;
	float radius = 0.2f, stength = 1.3f;
	bool enableSSAO = true;
};
