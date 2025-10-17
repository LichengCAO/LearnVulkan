#version 460 core
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT float d;

void main()
{
    d = gl_HitTEXT;
}