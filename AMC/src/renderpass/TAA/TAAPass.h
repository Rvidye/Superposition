#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>
#include<UBO.h>

class TAAPass : public AMC::RenderPass {

public:
	void create(AMC::RenderContext& context) override;
	void execute(AMC::Scene* scene, AMC::RenderContext& context) override;
	const char* getName() const override;
	void renderUI() override;

private:
	AMC::ShaderProgram* m_ProgramTAA = nullptr;

	GLuint m_texturePing = 0;
	GLuint m_texturePong = 0;
	bool m_writeToFirst = true;
	int m_frameIndex = 0;

	GLuint m_taaUBO = 0;
	TAADataUBO m_taaData;

	GLuint m_originalDeferredResult = 0;
	GLsizei m_width = 0;
	GLsizei m_height = 0;

	static float Halton(int index, int base);
};
