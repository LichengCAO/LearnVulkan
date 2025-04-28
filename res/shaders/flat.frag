#version 450

#extension GL_KHR_vulkan_glsl:enable

layout(set = 0, binding = 2) uniform sampler2D textureSampler;

layout (location = 0) in PerVertexData
{
	vec3 normal;
	vec2 uv;
};

layout (location = 0) out vec4 fs_Color;

void main()
{
	fs_Color = texture(textureSampler, uv);
}