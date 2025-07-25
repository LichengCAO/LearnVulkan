#version 460

//#extension GL_KHR_vulkan_glsl:enable

layout (location = 0) in vec4 vertexPosition;
layout (location = 1) in vec4 vertexNormal;
layout (location = 0) out vec3 fragPosition;
layout (location = 1) out vec3 fragNormal;

layout (set = 0, binding = 0) uniform CameraData
{
	mat4 viewProjectionMatrix;
} cameraData;

layout (set = 1, binding = 0) uniform ModelData
{
	mat4 modelMatrix;
	mat4 normalMatrix;
} modelData;

void main()
{
	gl_Position = cameraData.viewProjectionMatrix * modelData.modelMatrix * vertexPosition;
	fragNormal = vec3(normalize(modelData.normalMatrix * vertexNormal));
	fragPosition = (modelData.modelMatrix * vertexPosition).xyz;
}