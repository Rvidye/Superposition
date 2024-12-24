#pragma once
#include<RenderPass.h>
#include<ShaderProgram.h>

class DeferredPass : public AMC::RenderPass {

public:
	void create(AMC::RenderContext& context) override;
	void execute(AMC::Scene* scene, AMC::RenderContext& context) override;
	const char* getName() const override;
	void renderUI() override;
	AMC::ShaderProgram* m_ProgramDeferredLighting;
	GLuint m_FBO;
	GLuint m_TextureResult;
	bool vxgi = false;
};