#include<common.h>
#include "ConeTracer.h"

void ConeTracer::create(AMC::RenderContext& context) {
	m_programConeTrace = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\VXGI\\ConeTracer\\conetrace.comp")});

	glCreateTextures(GL_TEXTURE_2D, 1, &textureResultConeTrace);
	glTextureParameteri(textureResultConeTrace, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(textureResultConeTrace, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(textureResultConeTrace, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(textureResultConeTrace, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(textureResultConeTrace, 1, GL_RGBA16F, context.width, context.width);
	context.textureVXGIResult = textureResultConeTrace;
}

void ConeTracer::execute(AMC::Scene* scene, AMC::RenderContext& context) {

	if (!context.IsVGXI)
		return;
	////GBuffer
	//glBindTextureUnit(1, context.textureGBuffer[1]); // normal
	//glBindTextureUnit(2, context.textureGBuffer[2]); // metal-roughness
	//glBindTextureUnit(3, context.textureGBuffer[4]); // depth
	//glBindTextureUnit(0, context.textureAtmosphere); // skyAlbedo
	glBindTextureUnit(1, context.textureVolxelResult);
	glBindImageTexture(2, textureResultConeTrace, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

	m_programConeTrace->use();
	glUniform1i(m_programConeTrace->getUniformLocation("MaxSamples"), MaxSamples);
	glUniform1f(m_programConeTrace->getUniformLocation("StepMultiplier"), StepMultiplier);
	glUniform1f(m_programConeTrace->getUniformLocation("GIBoost"), AMC::GlobalGIBoost);
	glUniform1f(m_programConeTrace->getUniformLocation("GISkyBoxBoost"), GISkyBoxBoost);
	glUniform1f(m_programConeTrace->getUniformLocation("NormalRayOffset"), NormalRayOffset);
	GLuint workGroupSizeX = (context.width + 8 - 1) / 8;
	GLuint workGroupSizeY = (context.height + 8 - 1) / 8;
	glDispatchCompute( workGroupSizeX, workGroupSizeY, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
}

const char* ConeTracer::getName() const{
	return "Cone Trace";
}

void ConeTracer::renderUI()
{
#ifdef _MYDEBUG
	ImGui::SliderInt("MaxSamples", &MaxSamples, 1, 24);
	ImGui::SliderFloat("StepMultiplier", &StepMultiplier, 0.01f, 1.0f);
	if (ImGui::SliderFloat("GIBoost", &GIBoost, 0.0f, 5.0f)) {
		AMC::GlobalGIBoost = GIBoost;
	}
	ImGui::SliderFloat("GISkyBoxBoost", &GISkyBoxBoost, 0.0f, 5.0f);
	ImGui::SliderFloat("NormalRayOffset", &NormalRayOffset, 1.0f, 3.0f);
	if (ImGui::CollapsingHeader("Contracer Texture")) {
		ImGui::Image((void*)(intptr_t)textureResultConeTrace, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
	}
#endif
}