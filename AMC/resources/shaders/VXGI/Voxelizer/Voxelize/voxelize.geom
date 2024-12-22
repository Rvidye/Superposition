#version 460 core

//#extension GL_NV_geometry_shader_passthrough : require
//#extension GL_NV_geometry_shader_passthrough : enable

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in InOutData
{
    vec3 FragPos;
    vec2 TexCoord;
    vec3 Normal;
} inData[];

out OutData
{
    vec3 FragPos;
    vec2 TexCoord;
    vec3 Normal;
};

void main()
{
    vec3 p1 = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    vec3 normalWeights = abs(cross(p1, p2));

    int dominantAxis = normalWeights.y > normalWeights.x ? 1 : 0;
    dominantAxis = normalWeights.z > normalWeights[dominantAxis] ? 2 : dominantAxis;

    // Swizzle is applied by selecting a viewport
    // This works using the GL_NV_viewport_swizzle extension
    int viewportIndex = 2 - dominantAxis;
    for (int i = 0; i < 3; i++) 
    {
        gl_Position = gl_in[i].gl_Position;
        gl_ViewportIndex = viewportIndex;
        FragPos      = inData[i].FragPos;
        TexCoord     = inData[i].TexCoord;
        Normal       = inData[i].Normal;
        EmitVertex();
    }
    EndPrimitive();
}
