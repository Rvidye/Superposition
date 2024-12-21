#version 460 core 
layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

layout(location = 4)uniform int lightIndex;
layout(location = 5) uniform mat4 viewProj[6];

out vec4 FragPos;

void main(void) 
{
    for(int face = 0; face < 6; ++face)
    {
        gl_Layer = lightIndex * 6 + face;
        for(int i = 0; i < 3; ++i)
        {
            FragPos = gl_in[i].gl_Position;
            gl_Position = viewProj[face] * FragPos;
            EmitVertex();
        }    
        EndPrimitive();
    }
};