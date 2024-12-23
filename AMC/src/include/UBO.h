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
