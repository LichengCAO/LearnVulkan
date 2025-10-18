#version 450

layout(location = 0) in vec2 inTexCoord;
layout(set = 0, binding = 0) uniform sampler2D resultTexture;

layout(location = 0) out vec4 outColor;

void main()
{
    //outColor = vec4(pow(texture(resultTexture, inTexCoord).rgb, vec3(1.0f/2.2f)), 1.0f);
    outColor = texture(resultTexture, inTexCoord);
}