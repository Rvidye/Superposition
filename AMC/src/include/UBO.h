#pragma once

#include<GL/glew.h>

struct GBufferDataUBO{

	GLuint64 AlbedoAlphaTexture;
	GLuint64 NormalTexture;
	GLuint64 MetallicRoughnessTexture;
	GLuint64 EmissiveTexture;
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

struct alignas(16) GpuLight {
	glm::vec3 position; float intensity;
	glm::vec3 direction; float range;
	glm::vec3 color; float spotAngle;
	float spotExponent;
	int type = 2;
	int shadows = 0;
	int shadowMapIndex = -1;
	int active;
	float pad[2];
};

struct alignas(16) GpuShadow {
	glm::mat4 ProjViewMatrices[6];
	glm::vec3 Position;
	float NearPlane;
	float FarPlane;
	int LightIndex;
	GLuint64 NearestSampler;
	GLuint64 ShadowSampler;
	float padding[2];
};
