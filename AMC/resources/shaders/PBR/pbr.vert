#version 460 core

layout(location = 0)in vec3 vPos;
layout(location = 1)in vec3 vNor;
layout(location = 2)in vec2 vTex;
layout(location = 3)in vec3 vTangent;
layout(location = 4)in vec3 vBitangent;


layout(location = 1) uniform mat4 nodeMat; 
uniform mat4 mMat;
uniform mat4 vMat;
uniform mat4 pMat;

out vec3 oNor;
out vec3 oWorldPos;
out vec2 oTex;
out mat3 oTbn;
//out vec3 oTangent;
//out vec3 oBitanget;

void main(void)
{
    gl_Position = pMat * vMat * mMat * nodeMat * vec4(vPos,1.0);
    oWorldPos = vec3(mMat * nodeMat * vec4(vPos,1.0f));
    oTex = vTex;

    mat3 normalMat = mat3(mMat);
    //oNormal = normalize(vec3(mMat * nodeMat * vec4(vNor,1.0f)));
	vec3 tT = normalize(vec3(mMat * vec4(vTangent,0.0)));
	vec3 tB = normalize(vec3(mMat *  vec4(vBitangent,0.0)));
    oNor =normalize( vec3(mMat  * vec4(vNor,0.0)));

	mat3 tbn = mat3(tT,tB,oNor);
    oTbn = tbn;
}
