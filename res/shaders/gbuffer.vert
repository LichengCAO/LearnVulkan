#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(set = 0, binding = 0) uniform CameraInformation
{
    mat4 view;
    mat4 proj;
    vec4 eye;
} cameraInfo;
layout(set = 1, binding = 0) uniform ModelTransform
{
    mat4 model;
    mat4 normalModel;
} modelTransform;

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;

void main()
{
    vec4 wPos = modelTransform.model * vec4(inPos, 1.0);
    vec4 wNormal = modelTransform.normalModel * vec4(inNormal, 0.0f);
    gl_Position = cameraInfo.proj * cameraInfo.view * wPos;
    outUV = inUV;
    outPos = wPos.xyz;
    outNormal = normalize(wNormal.xyz);
}