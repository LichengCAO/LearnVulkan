#version 450

layout(location = 0) in vec3 inFragColor;
layout(location = 1) in vec2 inTexCoord;
layout(binding = 1) uniform sampler2D texSampler;
layout(set = 1, binding = 1) uniform sampler2D texDistortUV;

layout(location = 0) out vec4 outColor;

void main(){
    vec2 uv = clamp(texture(texDistortUV, inTexCoord).rg + inTexCoord, vec2(0,0), vec2(1,1));
    outColor = texture(texSampler, uv);
}