#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>

class GBufferPass : public AMC::RenderPass {

public:
	GBufferPass(AMC::VkContext* vkctx) : ctx(vkctx), m_ProgramGBuffer(nullptr), m_textureDepth({}) {}
	void create(AMC::RenderContext& context) override;
	void execute(AMC::Scene* scene, AMC::RenderContext& context) override;
	const char* getName() const override;
	void renderUI() override;
	AMC::ShaderProgram* m_ProgramGBuffer;

	AMC::VkContext* ctx;

	GLuint gbuffer = 0;
	GLuint m_textureAlbedoAlpha = 0;
	GLuint m_textureNormal = 0;
	GLuint m_textureMetallicRoughness = 0;
	GLuint m_textureEmissive = 0;
	AMC::Image m_textureDepth;
};