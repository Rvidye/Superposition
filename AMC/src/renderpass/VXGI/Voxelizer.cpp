#include<common.h>
#include "Voxelizer.h"

void Voxelizer::create(){

	m_ProgramClearTexture = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\model\\spv\\model.vert.spv"), RESOURCE_PATH("shaders\\model\\spv\\model.frag.spv") });
	m_ProgramVoxelize = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\model\\spv\\model.vert.spv"), RESOURCE_PATH("shaders\\model\\spv\\model.frag.spv") });
	m_ProgramMipMap = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\model\\spv\\model.vert.spv"), RESOURCE_PATH("shaders\\model\\spv\\model.frag.spv") });
	m_ProgramVisualizeDebug = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\model\\spv\\model.vert.spv"), RESOURCE_PATH("shaders\\model\\spv\\model.frag.spv") });

	SetSize(256,256,256);
	SetGridSize(glm::vec3(-28.0f, -3.0f, -17.0f), glm::vec3(28.0f, 20.0f, 17.0f));
}

void Voxelizer::execute(const AMC::Scene* scene){
	ClearTextures();
	Voxelize(scene);
	MipMap();
}

void Voxelizer::SetSize(int width, int height, int depth){

	this->width = width;
	this->height = height;
	this->depth = depth;

	glCreateTextures(GL_TEXTURE_3D, 1, &resultVoxels);
	glTextureStorage3D(resultVoxels, 1, GL_RGBA16F, width, height, depth);
	glTextureParameteri(resultVoxels, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(resultVoxels, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(resultVoxels, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(resultVoxels, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(resultVoxels, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTextureParameterf(resultVoxels, GL_TEXTURE_MAX_ANISOTROPY, 16.0f);
}

void Voxelizer::SetGridSize(glm::vec3 min, glm::vec3 max){
	GridMin = glm::min(min, min - glm::vec3(0.1f));
	GridMax = glm::max(max, max + glm::vec3(0.1f));
}

void Voxelizer::ClearTextures(){

	m_ProgramClearTexture->use();
	glBindImageTexture(0, resultVoxels, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glDispatchCompute((width + 4 - 1)/4,(height + 4 - 1)/4,(depth+4-1)/4);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void Voxelizer::Voxelize(const AMC::Scene* scene){
}

void Voxelizer::MipMap(){
}
