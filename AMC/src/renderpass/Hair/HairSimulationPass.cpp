#include<common.h>
#include<HairMeshAsset.h>

#include "HairSimulationPass.h"

void HairSimulationPass::create(AMC::RenderContext& context)
{
	(void)context;
	m_simulateProgram = new AMC::ShaderProgram({
		RESOURCE_PATH("shaders\\hair\\hair_simulate.comp")
	});
	m_rebuildProgram = new AMC::ShaderProgram({
		RESOURCE_PATH("shaders\\hair\\hair_rebuild.comp")
	});
}

void HairSimulationPass::execute(AMC::Scene* scene, AMC::RenderContext& context)
{
	if (!context.IsHair || scene == nullptr || scene->hairs.empty() || m_simulateProgram == nullptr || m_rebuildProgram == nullptr) {
		return;
	}

	m_timeAccumulator += static_cast<float>(AMC::deltaTime);
	bool hairMoved = false;

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	for (auto& [name, hair] : scene->hairs) {
		if (!hair.visible || hair.asset == nullptr || !hair.asset->IsValid() || !hair.asset->IsSimulationEnabled() || !hair.asset->HasSimulationState()) {
			continue;
		}

		hair.asset->UpdateSimulationUniforms(static_cast<float>(AMC::deltaTime), m_timeAccumulator);
		hair.asset->BindSimulation();

		m_simulateProgram->use();
		const GLuint controlPointCount = hair.asset->GetControlPointCount();
		glDispatchCompute((controlPointCount + 63u) / 64u, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		m_rebuildProgram->use();
		glBindImageTexture(0, hair.asset->GetHairMeshTexture(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		glBindImageTexture(1, hair.asset->GetUTexture(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		glBindImageTexture(2, hair.asset->GetVTexture(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		const GLuint rebuildDepth = (AMC::HairMeshSliceCount > AMC::HairStyleSliceCount)
			? static_cast<GLuint>(AMC::HairMeshSliceCount)
			: static_cast<GLuint>(AMC::HairStyleSliceCount);
		glDispatchCompute(
			(hair.asset->GetAtlasWidth() + 3u) / 4u,
			(hair.asset->GetAtlasHeight() + 3u) / 4u,
			rebuildDepth
		);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

		hair.asset->SwapSimulationState();
		hairMoved = true;
	}

	if (hairMoved) {
		context.dirtyFlags.rasterShadowDirty = true;
		context.dirtyFlags.voxelDirty = true;
		if (scene->lightManager != nullptr) {
			scene->lightManager->GetShadowManager()->InvalidateAllShadows();
		}
		scene->reCalculateSceneAABB();
	}
}

const char* HairSimulationPass::getName() const
{
	return "Hair Simulation";
}

void HairSimulationPass::renderUI()
{
#ifdef _MYDEBUG
	AMC::HairSimulationParams params = AMC::HairMeshAsset::GetGlobalSimulationParams();
	bool changed = false;

	if (ImGui::CollapsingHeader("Hair Simulation")) {
		changed |= ImGui::Checkbox("Enabled", &params.Enabled);
		changed |= ImGui::SliderFloat3("Gravity", &params.Gravity.x, -20.0f, 20.0f);
		changed |= ImGui::SliderFloat3("Wind Dir", &params.WindDirection.x, -1.0f, 1.0f);
		changed |= ImGui::SliderFloat("Wind Strength", &params.WindStrength, 0.0f, 2.0f);
		changed |= ImGui::SliderFloat("Wind Pulse", &params.WindPulseFrequency, 0.0f, 4.0f);
		changed |= ImGui::SliderFloat("Stiffness", &params.Stiffness, 0.0f, 64.0f);
		changed |= ImGui::SliderFloat("Damping", &params.Damping, 0.0f, 1.0f);
		changed |= ImGui::SliderFloat("Tip Influence", &params.TipInfluence, 0.0f, 2.0f);
		changed |= ImGui::SliderFloat("Max Displacement", &params.MaxDisplacement, 0.0f, 0.5f);
		changed |= ImGui::SliderFloat("Time Scale", &params.TimeScale, 0.0f, 2.0f);
	}

	if (changed) {
		AMC::HairMeshAsset::SetGlobalSimulationParams(params);
	}
#endif
}
