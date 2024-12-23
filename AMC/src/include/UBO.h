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