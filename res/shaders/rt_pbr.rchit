#version 460 core
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require
// https://stackoverflow.com/questions/60549218/what-use-has-the-layout-specifier-scalar-in-ext-scalar-block-layout
#extension GL_EXT_scalar_block_layout : enable

//#include "rt_pbr_common.glsl"
#include "rt_common.glsl"
#include "microfacet.glsl"

struct Material
{
    vec4 color;
    uvec4 type;
};

// Note that there is no requirement that the location of the callee's incoming payload match 
// the payload argument the caller passed to traceRayEXT! 
// This is quite unlike the in/out variables used to connect vertex shaders and fragment shaders.
layout(location = 1) rayPayloadInEXT PBRPayload payload;

layout(set = 0, binding = 1) uniform accelerationStructureEXT accelerationStructure;
layout(set = 0, binding = 3, scalar) buffer InstanceAddressData_ {
    InstanceAddressData i[];
} instanceAddressData;
layout(set = 1, binding = 0) buffer Materials
{
    Material i[];
} material;

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
vec3 ObjectToWorld(const vec3 pos)
{
    return gl_ObjectToWorldEXT * vec4(pos, 1.0f);
}
// Transforming the normal to world space
vec3 ObjectNormalToWorldNormal(const vec3 normal)
{
    return  normalize(vec3(normal * gl_WorldToObjectEXT));
}

vec3 Noise(uint n)
{
    return  random_pcg3d(uvec3(gl_LaunchIDEXT.xy, n));
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

    payload.rayOrigin = posWorld + nrmWorld * 0.001f; // Offset a little to avoid self-intersection

    if (material.i[gl_InstanceCustomIndexEXT].type.x == 1)
    {
        payload.traceEnd = true;
    }
    else if (material.i[gl_InstanceCustomIndexEXT].type.x == 2)
    {
        vec3 random = Noise(payload.randomSeed);
        float pdf = 1.0f;
        vec3 wi = vec3(0.0f);
        float roughness = 0.2f;
        SampleMicrofacetNormal(-payload.rayDirection, roughness, random.xy, wi, pdf);
        payload.hitValue = payload.hitValue * MicrofacetBRDF(-payload.rayDirection, wi, vec3(1.0f), roughness) * abs(dot(wi, nrmWorld)) / pdf; // The 100 factor is to boost the brightness a bit
        payload.rayDirection = wi;
    }
    else
    {
        vec3 random = Noise(payload.randomSeed);
        vec3 tangentSample = CosineWeightedSample(random.xy);
        payload.rayDirection = TangentToWorld(nrmWorld, -payload.rayDirection, tangentSample);
        // pdf = cos(theta) / pi
        // BRDF = baseColor / pi
        // lambert law = BRDF * cos(theta) / pdf = baseColor / pi * cos(theta) / (cos(theta) / pi) = baseColor
        payload.hitValue = payload.hitValue * material.i[gl_InstanceCustomIndexEXT].color.rgb; // Simple diffuse lighting
    }
}