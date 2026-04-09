#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>

class GBufferPass;

class HairGBufferPass : public AMC::RenderPass {
public:
	explicit HairGBufferPass(GBufferPass* gbufferPass) : m_gbufferPass(gbufferPass) {}

	void create(AMC::RenderContext& context) override;
	void execute(AMC::Scene* scene, AMC::RenderContext& context) override;
	const char* getName() const override;
	void renderUI() override;

private:
	GBufferPass* m_gbufferPass = nullptr;
	AMC::ShaderProgram* m_program = nullptr;
};
