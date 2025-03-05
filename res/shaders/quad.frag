#version 450

layout(location = 0) in vec2 inTexCoord;
layout(set = 0, binding = 0) uniform sampler2D texAlbedo;
layout(set = 0, binding = 1) uniform sampler2D texPos;
layout(set = 0, binding = 2) uniform sampler2D texNormal;
layout(set = 1, binding = 0) uniform sampler2D texOIT;
layout(set = 1, binding = 1) uniform sampler2D texDistortUV;

layout(location = 0) out vec4 outColor;

void main(){
    vec2 uv = clamp(texture(texDistortUV, inTexCoord).rg + inTexCoord, vec2(0, 0), vec2(1, 1));
    outColor = texture(texAlbedo, uv);
    vec4 oitColor = texture(texOIT, inTexCoord);
    outColor = vec4(oitColor.rgb + outColor.rgb * oitColor.a, 1.0f);
}