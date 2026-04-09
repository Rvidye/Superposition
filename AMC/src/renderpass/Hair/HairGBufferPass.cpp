#include<common.h>
#include<HairMeshAsset.h>

#include "../GBuffer/GBufferPass.h"
#include "HairGBufferPass.h"

void HairGBufferPass::create(AMC::RenderContext& context)
{
	m_program = new AMC::ShaderProgram({
		RESOURCE_PATH("shaders\\hair\\hair_gbuffer.task"),
		RESOURCE_PATH("shaders\\hair\\hair_gbuffer.mesh"),
		RESOURCE_PATH("shaders\\hair\\hair_gbuffer.frag")
	});
}

void HairGBufferPass::execute(AMC::Scene* scene, AMC::RenderContext& context)
{
	if (!context.IsHair || scene->hairs.empty() || m_gbufferPass == nullptr || m_program == nullptr) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, m_gbufferPass->gbuffer);
	glViewport(0, 0, context.width, context.height);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_CONSERVATIVE_RASTERIZATION_NV);
	glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	m_program->use();
	glBindVertexArray(context.emptyVAO);

	for (auto& [name, hair] : scene->hairs) {
		if (!hair.visible || hair.asset == nullptr || !hair.asset->IsValid()) {
			hair.prevMatrix = hair.matrix;
			continue;
		}

		hair.asset->Bind();
		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(hair.matrix));
		glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(hair.prevMatrix));
		glDrawMeshTasksNV(0, hair.asset->GetBundleCount());
		hair.prevMatrix = hair.matrix;
	}
}

const char* HairGBufferPass::getName() const
{
	return "Hair GBuffer";
}

void HairGBufferPass::renderUI()
{
#ifdef _MYDEBUG
	AMC::HairStyleParameters params = AMC::HairMeshAsset::GetGlobalStyleParameters();
	bool changed = false;

	if (ImGui::CollapsingHeader("Hair Styling", ImGuiTreeNodeFlags_DefaultOpen)) {
		changed |= ImGui::SliderFloat("Fuzz Amp", &params.FuzzAmplitude, 0.0f, 0.02f);
		changed |= ImGui::SliderFloat("Fuzz Exp", &params.FuzzAmplitudeExponent, 0.1f, 3.0f);
		changed |= ImGui::SliderFloat("Fuzz Dist", &params.FuzzDistributionExponent, 0.1f, 3.0f);

		changed |= ImGui::SliderFloat("Frizz Amp", &params.FrizzAmplitude, 0.0f, 0.04f);
		changed |= ImGui::SliderFloat("Frizz Exp", &params.FrizzAmplitudeExponent, 0.1f, 3.0f);
		changed |= ImGui::SliderFloat("Frizz Freq", &params.FrizzFrequency, 0.0f, 16.0f);

		changed |= ImGui::SliderFloat("Kink Amp", &params.KinkAmplitude, 0.0f, 0.04f);
		changed |= ImGui::SliderFloat("Kink Exp", &params.KinkAmplitudeExponent, 0.1f, 3.0f);
		changed |= ImGui::SliderFloat("Kink UV Freq", &params.KinkUVFrequency, 0.0f, 16.0f);
		changed |= ImGui::SliderFloat("Kink W Freq", &params.KinkWFrequency, 0.0f, 20.0f);

		changed |= ImGui::SliderFloat("Curl Amp", &params.CurlAmplitude, 0.0f, 0.06f);
		changed |= ImGui::SliderFloat("Curl Exp", &params.CurlAmplitudeExponent, 0.1f, 3.0f);
		changed |= ImGui::SliderFloat("Curl Bias", &params.CurlDistributionExponent, 0.1f, 3.0f);
		changed |= ImGui::SliderFloat("Curl Freq", &params.CurlFrequency, 0.0f, 12.0f);
		changed |= ImGui::SliderFloat("Curl Turns", &params.CurlRotations, 0.0f, 6.0f);

		changed |= ImGui::SliderFloat("Clump Thick", &params.ClumpThickness, 0.0f, 0.08f);
		changed |= ImGui::SliderFloat("Clump Exp", &params.ClumpExponent, 0.1f, 3.0f);
		changed |= ImGui::SliderFloat("Clump Noise", &params.ClumpNoiseAmount, 0.0f, 0.5f);
		changed |= ImGui::SliderFloat("Clump Noise Freq", &params.ClumpNoiseFrequency, 0.0f, 12.0f);
		changed |= ImGui::SliderFloat("Clump Grid", &params.ClumpGridResolution, 1.0f, 12.0f);
		changed |= ImGui::SliderFloat("Clump Percent", &params.ClumpPercentage, 0.0f, 1.0f);
		changed |= ImGui::SliderFloat("Clump Var", &params.ClumpVariation, 0.0f, 1.0f);
	}

	if (changed) {
		AMC::HairMeshAsset::SetGlobalStyleParameters(params);
	}
#endif
}
