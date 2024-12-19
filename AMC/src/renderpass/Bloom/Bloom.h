#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>

class Bloom : public AMC::RenderPass {

public:
	void create(AMC::RenderContext& context) override;
	void execute(const AMC::Scene* scene, AMC::RenderContext& context) override;
	AMC::ShaderProgram* m_ProgramBloom;
	float threshold;
	float maxColor;
	int minusLods;
	GLuint textureDownsample, textureUpsample, levels;
	GLsizei texWidth, texHeight;
};