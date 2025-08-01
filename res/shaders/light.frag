#version 450

layout(location = 0) in vec2 inTexCoord;
layout(set = 0, binding = 0) uniform sampler2D texAlbedo;
layout(set = 0, binding = 1) uniform sampler2D texPos;
layout(set = 0, binding = 2) uniform sampler2D texNormal;

layout(location = 0) out vec4 outColor;

#define MAX_MIP_LEVEL 9.0f

void main(){
    outColor = texture(texAlbedo, inTexCoord);
}