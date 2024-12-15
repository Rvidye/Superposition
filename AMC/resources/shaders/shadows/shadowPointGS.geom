#version 460 core 
layout (triangles) in;
layout (triangle_strip, max_vertices= 3) out;
layout(invocations = 6) in;

layout(location = 2) uniform mat4 viewProj[6];

void main(void) 
{
    gl_Layer = gl_InvocationID;

    gl_Position = viewProj[gl_InvocationID] * gl_in[0].gl_Position;
    EmitVertex();

    gl_Position = viewProj[gl_InvocationID] * gl_in[1].gl_Position;
    EmitVertex();

    gl_Position = viewProj[gl_InvocationID] * gl_in[2].gl_Position;
    EmitVertex();

    EndPrimitive();
};