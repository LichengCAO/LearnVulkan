#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColor;
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

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec3 vPosWorld;
layout(location = 2) out vec3 vNormalWorld;

void main()
{
    vec4 posWorld = modelTransform.model * vec4(inPos, 1.0f);
    gl_Position = cameraInfo.proj * cameraInfo.view * posWorld;
    vPosWorld = posWorld.xyz;
    vNormalWorld = normalize(vec3(modelTransform.normalModel * vec4(inNormal, 0.0f)));
    outColor = inColor;
}