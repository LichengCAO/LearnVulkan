#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 outNormal;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec3 outPos;

void main()
{
    outNormal = vec4(inNormal * 0.5f + vec3(0.5f, 0.5f, 0.5f), 1.0f);
    outUV = inUV;
    outPos = inPos;
}