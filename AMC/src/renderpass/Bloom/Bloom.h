#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>

class Bloom : public AMC::RenderPass {

public:
	void create(AMC::RenderContext& context) override;
	void execute(AMC::Scene* scene, AMC::RenderContext& context) override;
	const char* getName() const override;
	void renderUI() override;
	AMC::ShaderProgram* m_ProgramBloom;
	float threshold;
	float maxColor;
	int minusLods;
	bool enableBloom = true;
	GLuint textureDownsample, textureUpsample, levels;
	GLsizei texWidth, texHeight;
};