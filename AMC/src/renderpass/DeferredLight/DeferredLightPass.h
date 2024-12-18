#pragma once
#include<RenderPass.h>
#include<ShaderProgram.h>

class DeferredPass : public AMC::RenderPass {

public:
	void create(AMC::RenderContext& context) override;
	void execute(const AMC::Scene* scene, AMC::RenderContext& context) override;
	AMC::ShaderProgram* m_ProgramDeferredLighting;
	GLuint m_FBO;
	GLuint m_TextureResult;
};