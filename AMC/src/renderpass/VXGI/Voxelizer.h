#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>

class Voxelizer : public AMC::RenderPass {

	public:

		void create(AMC::RenderContext& context) override;
		void execute(const AMC::Scene* scene, AMC::RenderContext& context) override;
		void SetSize(int width, int height, int depth);
		void SetGridSize(glm::vec3 min, glm::vec3 max);
		void ClearTextures();
		void Voxelize(const AMC::Scene* scene);
		void MipMap();

		AMC::ShaderProgram* m_ProgramClearTexture;
		AMC::ShaderProgram* m_ProgramVoxelize;
		AMC::ShaderProgram* m_ProgramMipMap;
		AMC::ShaderProgram* m_ProgramVisualizeDebug;

		GLuint resultVoxels;
		int width, height, depth;
		glm::vec3 GridMin, GridMax;
		float debugConeAngle = 0.0f;
		float debugStepMultiplier = 0.4f;
};
