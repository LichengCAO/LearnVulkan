#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : enable

#include "rt_common.glsl"

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 0) out vec4 fs_Color;

layout (set = 0, binding = 1) uniform accelerationStructureEXT topLevelAS;
layout (set = 0, binding = 2) buffer InstanceAddressData_
{
    InstanceAddressData i[];
} instanceAddressData;

layout(buffer_reference, scalar) buffer Vertices {
    Vertex v[];
}; // Positions of an object
layout(buffer_reference, scalar) buffer Indices {
    int i[];
}; // Triangle indices

const vec3 posLight = vec3(2.0f, 1.0f, 50.0f); // Position of the light source

void main()
{
	vec3 normalWorld = normalize(vertexNormal);
	vec3 lightDir = normalize(posLight - vertexPosition);
	float tMin = 0.001f;
	float tMax = 100.0f;
	rayQueryEXT rayQuery;
	rayQueryInitializeEXT(
		rayQuery,
		topLevelAS,
		gl_RayFlagsTerminateOnFirstHitEXT, 
		0xFF,
		vertexPosition,
		tMin, 
		lightDir, 
		tMax);

	while (rayQueryProceedEXT(rayQuery))
	{

	}

	if (rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT)
	{
		fs_Color = vec4(0.0f, 0.0f, 0.0f, 1.0f); // No intersection, return black
	}
	else
	{
		float diffuse = max(dot(normalWorld, lightDir), 0.0f);
		fs_Color = vec4(diffuse, diffuse, diffuse, 1.0f); // Simple diffuse shading
	}
}