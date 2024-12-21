#version 460 core 
layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

layout(location = 2)uniform float far_plane;
layout(location = 3)uniform vec3 lightPos;
layout(location = 4) uniform mat4 viewProj[6];

out vec4 FragPos;

void main(void) 
{
    for(int face = 0; face < 6; ++face)
    {
        gl_Layer = face;
        for(int i = 0; i < 3; ++i)
        {
            FragPos = gl_in[i].gl_Position;
            gl_Position = viewProj[face] * FragPos;
            EmitVertex();
        }    
        EndPrimitive();
    }
};