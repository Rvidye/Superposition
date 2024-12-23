#pragma once

#include<GL/glew.h>

struct GBufferDataUBO{

	GLuint64 AlbedoAlphaTexture;
	GLuint64 NormalTexture;
	GLuint64 MetallicRoughnessTexture;
	GLuint64 EmissiveTexture;
	GLuint64 VelocityTexture;
	GLuint64 DepthTexture;
};

struct SkyBoxUBO {
	GLuint64 Albedo;
};

struct VoxelizerDataUBO {
	glm::vec3 GridMin;
	float _pad0;
	glm::vec3 GridMax;
	float _pad1;
};

struct GPUMaterial{

	glm::vec3 EmissiveFactor;
	GLuint BaseColorFactor;
	glm::vec3 Absorbance;
	float IOR;
	float TransmissionFactor;
	float RoughnessFactor;
	float MetallicFactor;
	float AlphaCutoff;
	GLuint64 BaseColor;
	GLuint64 MetallicRoughness;
	GLuint64 Normal;
	GLuint64 Emissive;
	GLuint64 Transmission;
	float _pad0 = 0.0f;
	float _pad1 = 0.0f;
};

