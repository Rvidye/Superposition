#version 460 core

#extension GL_ARB_bindless_texture : require
#extension GL_EXT_shader_image_load_formatted : require

const vec2 VertexPositions[] =
{
    vec2( -1.0, -1.0 ),
    vec2(  3.0, -1.0 ),
    vec2( -1.0,  3.0 )
};

out InOutData
{
    vec2 TexCoord;
} outData;

void main()
{
    gl_Position = vec4(VertexPositions[gl_VertexID], 0.0, 1.0);
    outData.TexCoord = gl_Position.xy * 0.5 + 0.5;
}