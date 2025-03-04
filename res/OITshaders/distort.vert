#version 450

layout(location = 0) in vec3 hPos;
layout(location = 1) in vec3 hNormal;
layout(set = 0, binding = 0) uniform CameraInformation
{
    mat4 view;
    mat4 proj;
} cameraInfo;
layout(set = 1, binding = 0) uniform ModelTransform
{
    mat4 model;
    mat4 normalModel; // inverse transpose of model matrix
} modelTransform;
layout(set = 2, binding = 0) uniform CameraViewInformation
{
	mat4 normalView; // inverse transpose of view matrix
	float InvTanHalfFov;
	float ScreenRatioXY;
} cameraView;

layout(location = 0) out vec3 vViewNormal;
layout(location = 1) out vec4 vScreenPos;

void main()
{
    vScreenPos = cameraInfo.proj * cameraInfo.view * modelTransform.model * vec4(hPos, 1.0f);
    vViewNormal = normalize(cameraView.normalView * modelTransform.normalModel * vec4(hNormal, 0.0f));
    gl_Position = vScreenPos;
}