#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>

class GBufferPass : public AMC::RenderPass {

public:
	void create() override;
	void execute(const AMC::Scene* scene) override;
	void debugGBuffer();
	AMC::ShaderProgram* m_ProgramGBuffer;

	GLuint gbuffer;
	GLuint m_textureAlbedoAlpha;
	GLuint m_textureNormal;
	GLuint m_textureMetallicRoughness;
	GLuint m_textureEmissive;
	GLuint m_textureDepth;
};