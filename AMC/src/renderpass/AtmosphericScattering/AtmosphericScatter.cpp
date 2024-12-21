#include<common.h>
#include<cmath>
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

void AtmosphericScatterer::execute(AMC::Scene* scene, AMC::RenderContext& context) {

	// Compute Atmospheric Scattering
	glBindImageTexture(0, textureAtmosphericResult, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	m_ProgramAtmosphericScatter->use();
	glUniform1i(m_ProgramAtmosphericScatter->getUniformLocation("ISteps"), ISteps);
	glUniform1i(m_ProgramAtmosphericScatter->getUniformLocation("JSteps"), JSteps);
	glUniform1f(m_ProgramAtmosphericScatter->getUniformLocation("LightIntensity"), LightIntensity);
	glUniform1f(m_ProgramAtmosphericScatter->getUniformLocation("Azimuth"), Azimuth);
	glUniform1f(m_ProgramAtmosphericScatter->getUniformLocation("Elevation"), Elevation);
	GLuint workGroupSizeX = (128 + 8 - 1) / 8;
	GLuint workGroupSizeY = (128 + 8 - 1) / 8;
	glDispatchCompute(workGroupSizeX, workGroupSizeY, 6);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
}

const char* AtmosphericScatterer::getName() const
{
	return "Atmospheric Scatter";
}

void AtmosphericScatterer::renderUI()
{
#ifdef _MYDEBUG
	ImGui::Checkbox("Enable Scattering", &enableAtmosphericScattering);
	ImGui::SliderFloat("Elevation", &Elevation, - glm::pi<float>(), glm::pi<float>());
	ImGui::SliderFloat("Azimuth", &Azimuth, -glm::pi<float>(), glm::pi<float>());
	ImGui::DragFloat("LightIntensity", &LightIntensity, 0.2f);
	ImGui::SliderInt("InScatteringSamples", &ISteps, 1, 100);
	ImGui::SliderInt("DensitySamples", &JSteps, 1, 40);
#endif
}
