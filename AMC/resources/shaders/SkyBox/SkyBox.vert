#version 460 core

#extension GL_ARB_bindless_texture : require
#extension GL_EXT_shader_image_load_formatted : require

#include<..\..\..\resources\shaders\include\CubeVertices.glsl>
#include<..\..\..\resources\shaders\include\CommonTypes.glsl>
#include<..\..\..\resources\shaders\include\StaticUniformBuffers.glsl>

out vec3 TexCoord;

void main()
{
    mat4 viewNoTranslation = perFrameDataUBO.View;
    viewNoTranslation[3] = vec4(0.0, 0.0, 0.0, 1.0);
    TexCoord = CubeVertices[gl_VertexID];
    gl_Position = (perFrameDataUBO.Projection * viewNoTranslation * vec4(TexCoord, 1.0));
    gl_Position = gl_Position.xyww;
}

