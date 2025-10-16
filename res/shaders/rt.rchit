#version 460 core
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require
// https://stackoverflow.com/questions/60549218/what-use-has-the-layout-specifier-scalar-in-ext-scalar-block-layout
#extension GL_EXT_scalar_block_layout : enable

#include "rt_common.glsl"

// Note that there is no requirement that the location of the callee's incoming payload match 
// the payload argument the caller passed to traceRayEXT! 
// This is quite unlike the in/out variables used to connect vertex shaders and fragment shaders.
layout(location = 1) rayPayloadInEXT HitPayload payload;

layout(set = 0, binding = 3, scalar) buffer InstanceAddressData_ {
    InstanceAddressData i[];
} instanceAddressData;

// https://stackoverflow.com/questions/70887022/resource-pointer-using-buffer-reference-doesnt-point-to-the-right-thing
layout(buffer_reference, scalar) buffer Vertices {
    Vertex v[];
}; // Positions of an object
layout(buffer_reference, scalar) buffer Indices {
    int i[];
}; // Triangle indices

hitAttributeEXT vec2 attribs; // Barycentric coordinates

vec3 GetBarycentrics()
{
   // Barycentric coordinates are passed as hit attributes
    return vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
}

// gl_ObjectToWorldEXT and gl_WorldToObjectEXT are 3 row 4 column matrices
// Transforming the position to world space
vec3 ObjectToWorld(const vec3 pos)
{
    return gl_ObjectToWorldEXT * vec4(pos, 1.0f);
}
// Transforming the normal to world space
vec3 ObjectNormalToWorldNormal(const vec3 normal)
{
    return  normalize(vec3(normal * gl_WorldToObjectEXT));
}
vec3 GetRayHitPosition()
{
    return gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
}

void main()
{
    InstanceAddressData instanceData = instanceAddressData.i[gl_InstanceCustomIndexEXT];
    Vertices vertices = Vertices(instanceData.vertexAddress);
    Indices indices = Indices(instanceData.indexAddress);

    const int indexOffset = gl_PrimitiveID * 3;
    const vec3 barycentrics = GetBarycentrics();
    
    Vertex v0 = vertices.v[indices.i[indexOffset + 0]];
    Vertex v1 = vertices.v[indices.i[indexOffset + 1]];
    Vertex v2 = vertices.v[indices.i[indexOffset + 2]];

    const vec3 posObject = v0.position.xyz * barycentrics.x + v1.position.xyz * barycentrics.y + v2.position.xyz * barycentrics.z;
    const vec3 posWorld = ObjectToWorld(posObject);
    
    const vec3 nrmObject = v0.normal.xyz * barycentrics.x + v1.normal.xyz * barycentrics.y + v2.normal.xyz * barycentrics.z;
    const vec3 nrmWorld = ObjectNormalToWorldNormal(nrmObject); // GetRayHitPosition() can also be used to get the world position, but is less precise

    payload.hitValue = nrmWorld * 0.5f + vec3(0.5f, 0.5f, 0.5f); // Convert to [0, 1] range for color output
} 