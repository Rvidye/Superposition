#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>

class Voxelizer : public AMC::RenderPass {

	public:

		void create(AMC::RenderContext& context) override;
		void execute(AMC::Scene* scene, AMC::RenderContext& context) override;
		const char* getName() const override;
		void renderUI() override;
		void SetSize(int width, int height, int depth);
		void SetGridSize(glm::vec3 min, glm::vec3 max);
		void ClearTextures();
		void Voxelize(const AMC::Scene* scene);
		void VoxelizeMesh(const AMC::Scene* scene, bool useMeshShaders);
		void VoxelizeHair(const AMC::Scene* scene);
		void MipMap();
		void DubugVoxels(AMC::RenderContext& context);

		AMC::ShaderProgram* m_ProgramClearTexture = nullptr;
		AMC::ShaderProgram* m_ProgramVoxelize = nullptr;
		AMC::ShaderProgram* m_ProgramVoxelizeMesh = nullptr; // mesh pipeline path
		AMC::ShaderProgram* m_ProgramHairVoxelize = nullptr;
		AMC::ShaderProgram* m_ProgramMipMap = nullptr;
		AMC::ShaderProgram* m_ProgramVisualizeDebug = nullptr;

		GLuint resultVoxels = 0, debugResult = 0, voxelUBO = 0, tmpFBO = 0;
		int width = 0, height = 0, depth = 0, levels = 0;
		VoxelizerDataUBO GridData;
		float debugConeAngle = 0.0f;
		float debugStepMultiplier = 0.4f;
		bool debugVoxels = false;
};
