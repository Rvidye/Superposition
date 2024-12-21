#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>

class GBufferPass : public AMC::RenderPass {

public:
	void create(AMC::RenderContext& context) override;
	void execute(AMC::Scene* scene, AMC::RenderContext& context) override;
	const char* getName() const override;
	void renderUI() override;
	AMC::ShaderProgram* m_ProgramGBuffer;

	GLuint gbuffer;
	GLuint m_textureAlbedoAlpha;
	GLuint m_textureNormal;
	GLuint m_textureMetallicRoughness;
	GLuint m_textureEmissive;
	GLuint m_textureDepth;
};