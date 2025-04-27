#version 450

#extension GL_KHR_vulkan_glsl:enable

layout (location = 0) in PerVertexData
{
	vec3 color;
} fragIn;

layout (location = 0) out vec4 fs_Color;

void main()
{
	fs_Color = vec4(fragIn.color, 1.0f);
}