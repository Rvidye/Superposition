#include<common.h>
#include "Voxelizer.h"

void Voxelizer::create(AMC::RenderContext& context){

	m_ProgramClearTexture = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\VXGI\\Voxelizer\\Clear\\Clear.comp")});
	m_ProgramVoxelize = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\VXGI\\Voxelizer\\Voxelize\\Voxelize.vert"), RESOURCE_PATH("shaders\\VXGI\\Voxelizer\\Voxelize\\Voxelize.frag"), RESOURCE_PATH("shaders\\VXGI\\Voxelizer\\Voxelize\\Voxelize.geom") });
	m_ProgramMipMap = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\VXGI\\Voxelizer\\Mipmap\\mipmap.comp")});
	m_ProgramVisualizeDebug = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\VXGI\\Voxelizer\\Debug\\debug.comp") });

	SetSize(256,256,256);
	SetGridSize(glm::vec3(-5.0f, -5.0f, -5.0f), glm::vec3(5.0f, 5.0f, 5.0f));

	context.textureVolxelResult = resultVoxels;

	glCreateTextures(GL_TEXTURE_2D, 1, &debugResult);
	glTextureParameteri(debugResult, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(debugResult, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(debugResult, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(debugResult, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(debugResult, 1, GL_RGBA16F, 512, 512);

	glCreateBuffers(1, &voxelUBO);
	glNamedBufferData(voxelUBO, sizeof(VoxelizerDataUBO), nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 5, voxelUBO);
}

void Voxelizer::execute(AMC::Scene* scene, AMC::RenderContext& context){

	if (!context.IsVGXI)
		return;
	// Maybe we should take scene aabb into accound as well.
	float granularity = 8.0f;
	glm::vec3 quantizedMin = ((AMC::currentCamera->getViewPosition() - glm::vec3(35.0f, 20.0f, 35.0f)) / granularity)* granularity;
	glm::vec3 quantizedMax = ((AMC::currentCamera->getViewPosition() + glm::vec3(35.0f, 20.0f, 35.0f)) / granularity) * granularity;

	GridData.GridMin = quantizedMin;
	GridData.GridMax = quantizedMax;

	glNamedBufferSubData(voxelUBO, 0, sizeof(VoxelizerDataUBO), &GridData);

	ClearTextures();
	Voxelize(scene);
	MipMap();
	if (debugVoxels) {
		DubugVoxels(context);
	}
}

const char* Voxelizer::getName() const
{
	return "Voxelizer";
}

void Voxelizer::renderUI()
{
#ifdef _MYDEBUG
	ImGui::Checkbox("Debug Voxel", &debugVoxels);
	if (debugVoxels) {
		ImGui::DragFloat3("Grid Min", &GridData.GridMin.x, 0.1f);
		ImGui::DragFloat3("Grid Max", &GridData.GridMax.x, 0.1f);
		ImGui::SliderFloat("DebugStepMultiplier", &debugStepMultiplier, 0.05f, 1.0f);
		ImGui::SliderFloat("DebugConeAngle", &debugConeAngle, 0, 0.5f);
		if (ImGui::CollapsingHeader("Voxelizer Texture")) {
			ImGui::Image((void*)(intptr_t)debugResult, ImVec2(256, 256), ImVec2(0,1), ImVec2(1,0));
		}
	}
#endif
}

void Voxelizer::SetSize(int width, int height, int depth){

	this->width = width;
	this->height = height;
	this->depth = depth;
	levels = AMC::GetMaxMipmapLevel(width, height, depth);

	glCreateTextures(GL_TEXTURE_3D, 1, &resultVoxels);
	glTextureParameteri(resultVoxels, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(resultVoxels, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(resultVoxels, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(resultVoxels, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(resultVoxels, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTextureParameterf(resultVoxels, GL_TEXTURE_MAX_ANISOTROPY, 16.0f);
	glTextureStorage3D(resultVoxels, levels, GL_RGBA16F, width, height, depth);
}

void Voxelizer::SetGridSize(glm::vec3 min, glm::vec3 max){
	GridData.GridMin = glm::min(min, min - glm::vec3(0.1f));
	GridData.GridMax = glm::max(max, max + glm::vec3(0.1f));
}

void Voxelizer::ClearTextures(){

	glBindImageTexture(0, resultVoxels, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
	m_ProgramClearTexture->use();
	glDispatchCompute((width + 4 - 1)/4,(height + 4 - 1)/4,(depth+4-1)/4);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void Voxelizer::Voxelize(const AMC::Scene* scene){
	//glClipControl()
	// Set Viewport Swizzle for Geomtry Shader
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
	glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
	glViewportSwizzleNV(1, 0x9350, 0x9354, 0x9352, 0x9356);
	glViewportSwizzleNV(2, 0x9354, 0x9352, 0x9354, 0x9356);
	glViewportIndexedf(0, 0.0f,0.0f, (float)width,(float)height);
	glViewportIndexedf(1, 0.0f, 0.0f, (float)width, (float)height);
	glViewportIndexedf(2, 0.0f, 0.0f, (float)width, (float)height);
	glViewport(0, 0, 256, 256);
	glBindImageTexture(0, resultVoxels, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
	//glBindTextureUnit(8, scene->lightManager->GetShadowManager()->getPointShadowCubemap());
	scene->lightManager->BindUBO();
	m_ProgramVoxelize->use();
	for (const auto& [name, obj] : scene->models) {
		if (!obj.visible)
			continue;

		glUniformMatrix4fv(m_ProgramVoxelize->getUniformLocation("modelMat"), 1, GL_FALSE, glm::value_ptr(obj.matrix));
		obj.model->draw(m_ProgramVoxelize);
	}
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
}

void Voxelizer::MipMap() {
	glBindTextureUnit(0, resultVoxels);
	m_ProgramMipMap->use();
	for (int i = 1; i < levels; i++) {
		glUniform1i(m_ProgramMipMap->getUniformLocation("Lod"),i-1);

		glm::ivec3 size = AMC::GetMipmapLevelSize(width, height, depth, i);
		glBindImageTexture(0, resultVoxels, i, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
		glDispatchCompute((size.x + 4 - 1) / 4, (size.y + 4 - 1) / 4, (size.z + 4 - 1) / 4);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	}
}

void Voxelizer::DubugVoxels(AMC::RenderContext& context) {

	glBindImageTexture(0, debugResult, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
	glBindTextureUnit(0, resultVoxels);
	glBindTextureUnit(1, context.textureAtmosphere);
	m_ProgramVisualizeDebug->use();
	glUniform1f(m_ProgramVisualizeDebug->getUniformLocation("StepMultiplier"), debugStepMultiplier);
	glUniform1f(m_ProgramVisualizeDebug->getUniformLocation("ConeAngle"), debugConeAngle);
	glDispatchCompute((512 + 8 - 1) / 8, (512 + 8 - 1) / 8, 1);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
}
