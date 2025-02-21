#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(set = 0, binding = 0) uniform UBO
{
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 normalModel;
} ubo;

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;

void main()
{
    vec4 wPos = ubo.model * vec4(inPos, 1.0);
    vec4 wNormal = ubo.normalModel * vec4(inNormal, 0.0f);
    gl_Position = ubo.proj * ubo.view * wPos;
    outUV = inUV;
    outPos = wPos.xyz;
    outNormal = wNormal.xyz;
}