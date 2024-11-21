#version 460 core 

layout(location = 0)in vec3 vPos;
layout(location = 1)in vec3 vNor;
layout(location = 2)in vec2 vTex;
layout(location = 3)in vec3 vTangent;
layout(location = 4)in vec3 vBitangent;
layout(location = 5)in ivec4 vBoneIds;
layout(location = 6)in vec4 vWeights;

layout(location = 0) uniform mat4 mvpMat;
layout(location = 1) uniform mat4 nodeMat;
layout(location = 2) uniform mat4 bMat[100];

out vec2 oTex;

void main(void) 
{  
    vec4 totalPosition = vec4(0.0);
	for(int i = 0 ; i < 4; i++) {
		if(vBoneIds[i] == -1) {
			continue;
		}
		vec4 localPosition = bMat[vBoneIds[i]] *vec4(vPos,1.0);
		totalPosition += localPosition * vWeights[i];
	}
    gl_Position = mvpMat * totalPosition; 
    oTex = vTex;
};