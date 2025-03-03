#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(set = 1, binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outPos;
layout(location = 2) out vec4 outNormal;

void main()
{
    outAlbedo = texture(texSampler, inUV);
    outNormal = vec4(inNormal * 0.5f + vec3(0.5f, 0.5f, 0.5f), 1.0f);
    outPos = vec4(inPos, 1.0f);
}