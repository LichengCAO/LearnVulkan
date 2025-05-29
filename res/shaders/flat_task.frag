#version 450

#extension GL_KHR_vulkan_glsl:enable

layout (location = 0) in PerVertexData
{
	vec3 colorRGB;
};

layout (location = 0) out vec4 fs_Color;

void main()
{
	fs_Color = vec4(colorRGB, 1.0);
}