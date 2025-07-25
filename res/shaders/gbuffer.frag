#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(set = 1, binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outPos;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out vec4 outDepth;

void main()
{
    outAlbedo = texture(texSampler, inUV);
    outNormal = vec4(normalize(inNormal), 1.0f);
    outPos = vec4(inPos, 1.0f);
    outDepth = vec4(1.0f / gl_FragCoord.w, gl_FragCoord.z, 0.0, 1.0);
}