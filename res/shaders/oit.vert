#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColor;
layout(set = 0, binding = 0) uniform CameraInformation
{
    mat4 view;
    mat4 proj;
} cameraInfo;
layout(set = 1, binding = 0) uniform ModelTransform
{
    mat4 model;
    mat4 normalModel;
} modelTransform;

layout(location = 0) out vec4 outColor;
layout(location = 1) out float vDepth;

void main()
{
    vec4 viewPos = cameraInfo.view * modelTransform.model * vec4(inPos, 1.0f);
    gl_Position = cameraInfo.proj * viewPos;
    vDepth = viewPos.z;
    outColor = inColor;
}