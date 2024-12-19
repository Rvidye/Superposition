#include<common.h>
#include "AtmosphericScatter.h"

void AtmosphericScatterer::create(AMC::RenderContext& context) {

	m_ProgramAtmosphericScatter = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\AtmosphericScattering\\AtmosphericScatter.comp") });
	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &textureAtmosphericResult);
	glTextureParameteri(textureAtmosphericResult, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(textureAtmosphericResult, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(textureAtmosphericResult, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(textureAtmosphericResult, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(textureAtmosphericResult, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTextureParameteri(textureAtmosphericResult, GL_TEXTURE_BASE_LEVEL, 0);
	glTextureStorage2D(textureAtmosphericResult, 1, GL_RGBA32F, 128, 128);

	context.textureAtmosphere = textureAtmosphericResult;
}

void AtmosphericScatterer::execute(const AMC::Scene* scene, AMC::RenderContext& context) {

	// Compute Atmospheric Scattering
	glBindImageTexture(0, textureAtmosphericResult, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	m_ProgramAtmosphericScatter->use();
	glUniform1i(m_ProgramAtmosphericScatter->getUniformLocation("ISteps"), 40);
	glUniform1i(m_ProgramAtmosphericScatter->getUniformLocation("JSteps"), 8);
	glUniform1f(m_ProgramAtmosphericScatter->getUniformLocation("LightIntensity"), 15.0f);
	glUniform1f(m_ProgramAtmosphericScatter->getUniformLocation("Azimuth"), 0.0f);
	glUniform1f(m_ProgramAtmosphericScatter->getUniformLocation("Elevation"), 0.0f);
	GLuint workGroupSizeX = (128 + 8 - 1) / 8;
	GLuint workGroupSizeY = (128 + 8 - 1) / 8;
	glDispatchCompute(workGroupSizeX, workGroupSizeY, 6);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
}
