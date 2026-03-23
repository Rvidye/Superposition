#include<common.h>
#include "TAAPass.h"

float TAAPass::Halton(int index, int base)
{
	float result = 0.0f;
	float f = 1.0f / (float)base;
	int i = index;
	while (i > 0) {
		result += f * (float)(i % base);
		i /= base;
		f /= (float)base;
	}
	return result;
}

void TAAPass::create(AMC::RenderContext& context)
{
	m_ProgramTAA = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\TAA\\TAAResolve.comp") });

	m_width = context.width;
	m_height = context.height;

	// Create ping-pong RGBA16F textures
	glCreateTextures(GL_TEXTURE_2D, 1, &m_texturePing);
	glTextureParameteri(m_texturePing, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(m_texturePing, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(m_texturePing, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_texturePing, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(m_texturePing, 1, GL_RGBA16F, m_width, m_height);

	glCreateTextures(GL_TEXTURE_2D, 1, &m_texturePong);
	glTextureParameteri(m_texturePong, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(m_texturePong, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(m_texturePong, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_texturePong, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(m_texturePong, 1, GL_RGBA16F, m_width, m_height);

	// Save original deferred result texture
	m_originalDeferredResult = context.textureDeferredResult;

	// Create TAA UBO at binding 7
	m_taaData.Jitter = glm::vec2(0.0f);
	m_taaData.SampleCount = 8;
	m_taaData.MipmapBias = 0.0f;

	glCreateBuffers(1, &m_taaUBO);
	glNamedBufferData(m_taaUBO, sizeof(TAADataUBO), &m_taaData, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 7, m_taaUBO);
}

void TAAPass::execute(AMC::Scene* scene, AMC::RenderContext& context)
{
	// Always restore original deferred result so other passes are unaffected
	context.textureDeferredResult = m_originalDeferredResult;

	if (!context.IsTAA) {
		context.taaJitter = glm::vec2(0.0f);
		return;
	}

	// Determine ping-pong: output is where we write, history is last frame's result
	GLuint outputTex = m_writeToFirst ? m_texturePing : m_texturePong;
	GLuint historyTex = m_writeToFirst ? m_texturePong : m_texturePing;

	// Update TAA UBO with current jitter
	int sampleIndex = m_frameIndex % m_taaData.SampleCount;
	glm::vec2 halton = glm::vec2(
		Halton(sampleIndex + 1, 2),
		Halton(sampleIndex + 1, 3)
	);
	// Remap [0,1] to [-1,1], then to subpixel NDC offset
	m_taaData.Jitter = (halton - 0.5f) * 2.0f / glm::vec2((float)m_width, (float)m_height);
	m_taaData.MipmapBias = -log2f((float)m_taaData.SampleCount) * 0.5f;

	glNamedBufferSubData(m_taaUBO, 0, sizeof(TAADataUBO), &m_taaData);

	// Bind compute shader resources
	m_ProgramTAA->use();
	glBindImageTexture(0, outputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glBindTextureUnit(0, historyTex);                   // history
	glBindTextureUnit(1, m_originalDeferredResult);     // current color

	GLuint workGroupsX = (m_width + 7) / 8;
	GLuint workGroupsY = (m_height + 7) / 8;
	glDispatchCompute(workGroupsX, workGroupsY, 1);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	// Redirect Tonemap to read from TAA output
	context.textureDeferredResult = outputTex;

	// Swap ping-pong and advance frame
	m_writeToFirst = !m_writeToFirst;
	m_frameIndex++;

	// Compute NEXT frame's jitter and store in context for RenderFrame to apply
	int nextSampleIndex = (m_frameIndex % m_taaData.SampleCount);
	glm::vec2 nextHalton = glm::vec2(
		Halton(nextSampleIndex + 1, 2),
		Halton(nextSampleIndex + 1, 3)
	);
	context.taaJitter = (nextHalton - 0.5f) * 2.0f / glm::vec2((float)m_width, (float)m_height);
}

const char* TAAPass::getName() const
{
	return "TAA Pass";
}

void TAAPass::renderUI()
{
#ifdef _MYDEBUG
	ImGui::SliderInt("TAA Samples", &m_taaData.SampleCount, 2, 32);
#endif
}
