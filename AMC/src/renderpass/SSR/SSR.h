#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>

class SSR : public AMC::RenderPass {

public:
	void create(AMC::RenderContext& context) override;
	void execute(const AMC::Scene* scene, AMC::RenderContext& context) override;
	AMC::ShaderProgram* m_ProgramSSR;
	AMC::ShaderProgram* m_ProgramMergeTextures;
	GLuint textureSSR;
};
