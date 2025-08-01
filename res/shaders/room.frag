#version 450

layout(set = 0, binding = 1) uniform sampler2D uSampler;
layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(uSampler, inUV);
}