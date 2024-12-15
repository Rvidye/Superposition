#version 460 core 
layout (triangles) in;
layout (triangle_strip, max_vertices= 3) out;

layout(location = 2) uniform mat4 viewProj[3];
layout(location = 5) uniform int numShadows;

void main(void) 
{
    for(int i = 0; i < numShadows; i++){
        gl_Layer = i;
        gl_Position = viewProj[i] * gl_in[0].gl_Position;
        EmitVertex();

        gl_Position = viewProj[i] * gl_in[1].gl_Position;
        EmitVertex();

        gl_Position = viewProj[i] * gl_in[2].gl_Position;
        EmitVertex();

        EndPrimitive();
    }
};