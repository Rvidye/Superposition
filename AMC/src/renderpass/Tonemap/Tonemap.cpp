#include<common.h>
#include "Tonemap.h"

void Tonemap::create(AMC::RenderContext& context) {

	m_ProgramTonemap = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\Tonemap\\TonemapGamma.comp")});

	glCreateTextures(GL_TEXTURE_2D, 1, &textureTonemapResult);
	glTextureParameteri(textureTonemapResult, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(textureTonemapResult, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(textureTonemapResult, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(textureTonemapResult, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(textureTonemapResult, 1, GL_RGBA8, context.width, context.width);
	context.textureTonemapResult = textureTonemapResult;
}

void Tonemap::execute(const AMC::Scene* scene, AMC::RenderContext& context) {

	glBindImageTexture(0, textureTonemapResult, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
	glBindTextureUnit(0, context.textureDeferredResult); // Rasterier Result
	glBindTextureUnit(1, context.textureBloomResult); // Bloom Result
	glBindTextureUnit(2, 0); // Volumetric Result
	m_ProgramTonemap->use();
	glUniform1f(0,Exposure);
	glUniform1f(1, Saturation);
	glUniform1f(2, Linear);
	glUniform1f(3, Peak);
	glUniform1f(4, Compression);
	glUniform1i(5, IsAgXTonemaping ? 1 : 0);
	GLuint workGroupSizeX = (context.width + 8 - 1) / 8;
	GLuint workGroupSizeY = (context.height + 8 - 1) / 8;
	glDispatchCompute(workGroupSizeX, workGroupSizeY, 1);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
}