#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>

class SSR : public AMC::RenderPass {

public:
	void create(AMC::RenderContext& context) override;
	void execute(const AMC::Scene* scene, AMC::RenderContext& context) override;
	const char* getName() const override;
	void renderUI() override;
	AMC::ShaderProgram* m_ProgramSSR;
	AMC::ShaderProgram* m_ProgramMergeTextures;
	GLuint textureSSR;
	int SampleCount = 30;
	int BinarySearchCount = 8;
	float MaxDist = 50.0f;
	bool enableSSR = true;
};
