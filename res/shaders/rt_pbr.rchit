#version 460 core
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require
// https://stackoverflow.com/questions/60549218/what-use-has-the-layout-specifier-scalar-in-ext-scalar-block-layout
#extension GL_EXT_scalar_block_layout : enable

#include "pbr_common.glsl"
#include "rt_common.glsl"
#include "pbr_microfacet.glsl"
#include "pbr_fresnel.glsl"
#include "pbr_volume_scattering.glsl"

struct Material
{
    vec4 color;
    uvec4 type;
};

// Note that there is no requirement that the location of the callee's incoming payload match 
// the payload argument the caller passed to traceRayEXT! 
// This is quite unlike the in/out variables used to connect vertex shaders and fragment shaders.
layout(location = 1) rayPayloadInEXT PBRPayload payload;
layout(location = 0) rayPayloadEXT float payloadT;

layout(set = 0, binding = 1) uniform accelerationStructureEXT accelerationStructure;
layout(set = 0, binding = 3, scalar) buffer InstanceAddressData_ {
    InstanceAddressData i[];
} instanceAddressData;
layout(set = 1, binding = 0) buffer Materials
{
    Material i[];
} material;
layout(push_constant) uniform shaderInformation
{
    layout(offset = 4)float a;
    float s;
    float g;
} sigma;

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

uint ApplyVolumeScattering(uint seed, inout vec3 rayOrigin, inout vec3 rayDirection, inout vec3 throughput)
{
    uint state = 0;

    SampleVolume(
        random_pcg3d(uvec3(gl_LaunchIDEXT.xy, seed)),
        random_pcg3d(uvec3(seed, gl_LaunchIDEXT.xy)),
        gl_HitTEXT,         // distance from ray origin to surface hit
        sigma.g,            // HenyeyGreenstein
        sigma.a,            //in float sigma_a,
        sigma.s,            //in float sigma_s,
        0,
        throughput, // total light
        rayOrigin,
        rayDirection,
        state);

    return state;
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

    if (material.i[gl_InstanceCustomIndexEXT].type.x == 1)
    {
        payload.traceEnd = true;
    }
    else if (material.i[gl_InstanceCustomIndexEXT].type.x == 2)
    {
        vec3 random = Noise(payload.randomSeed);
        float pdf = 1.0f;
        vec3 wm = vec3(0.0f);
        float roughness = 0.2f;
        vec3 tangentWo = WorldToTangent(nrmWorld, -payload.rayDirection);
        SampleMicrofacetNormal(tangentWo, roughness, random.xy, wm, pdf);
        payload.hitValue = payload.hitValue * MicrofacetBRDF(tangentWo, wm, roughness) * abs(wm.z) / pdf * material.i[gl_InstanceCustomIndexEXT].color.rgb;
        vec3 wi = Reflect(tangentWo, wm);
        payload.rayDirection = TangentToWorld(nrmWorld, wi);
        payload.rayOrigin = posWorld + sign(dot(nrmWorld, payload.rayDirection)) * 0.001f * nrmWorld; // Offset a little to avoid self-intersection
    }
    else if (material.i[gl_InstanceCustomIndexEXT].type.x == 8)
    {
        vec3 random = Noise(payload.randomSeed);
        float IORt = 1.514f;

        float fresnel = Fresnel(dot(-payload.rayDirection, nrmWorld), 1.0f, IORt);
        if (random.r < fresnel) // Reflect
        {
            payload.rayDirection = Reflect(-payload.rayDirection, nrmWorld);
            payload.hitValue = payload.hitValue * material.i[gl_InstanceCustomIndexEXT].color.rgb;
        }
        else // Refract
        {
            payload.rayDirection = Refract(-payload.rayDirection, nrmWorld, 1.0f, IORt);
            payload.hitValue = payload.hitValue * material.i[gl_InstanceCustomIndexEXT].color.rgb;
        }
        payload.rayOrigin = posWorld + sign(dot(nrmWorld, payload.rayDirection)) * 0.001f * nrmWorld; // Offset a little to avoid self-intersection
    }
    else if (material.i[gl_InstanceCustomIndexEXT].type.x == 7)
    {
        vec3 random = Noise(payload.randomSeed);
        float pdf = 1.0f;
        vec3 wm = vec3(0.0f);
        float roughness = 0.1f;
        vec3 tangentWo = WorldToTangent(nrmWorld, -payload.rayDirection);
        SampleMicrofacetNormal(tangentWo, roughness, random.xy, wm, pdf);
        float cosTheta = clamp(dot(tangentWo, wm), 0.0f, 1.0f);
        vec3 fresnel = vec3(1.0f);
        fresnel.r = Fresnel(cosTheta, Complex(1, 0), Complex(0.18f, 3.42f));
        fresnel.g = Fresnel(cosTheta, Complex(1, 0), Complex(0.42f, 2.35f));
        fresnel.b = Fresnel(cosTheta, Complex(1, 0), Complex(1.37f, 1.77f));
        //payload.hitValue = payload.hitValue * MicrofacetBRDF(tangentWo, wm, roughness) * abs(wm.z) / pdf * fresnel;
        payload.hitValue = payload.hitValue * fresnel;
        vec3 wi = Reflect(tangentWo, wm);
        payload.rayDirection = TangentToWorld(nrmWorld, wi);
        //payload.rayDirection = Reflect(-payload.rayDirection, nrmWorld);
        payload.rayOrigin = posWorld + sign(dot(nrmWorld, payload.rayDirection)) * 0.001f * nrmWorld; // Offset a little to avoid self-intersection
    }
    else if (material.i[gl_InstanceCustomIndexEXT].type.x == 9)
    {
        vec3 random = Noise(payload.randomSeed);
        float IORt = 1.514f;
        float fresnel = Fresnel(dot(-payload.rayDirection, nrmWorld), 1.0f, IORt);

        if (!payload.volumeScatter)
        {
            bool inside = dot(nrmWorld, payload.rayDirection) > 0.0f;
            if (random.r < fresnel) // Reflect
            {
                payload.rayDirection = Reflect(-payload.rayDirection, nrmWorld);
                payload.hitValue = payload.hitValue * material.i[gl_InstanceCustomIndexEXT].color.rgb;
                payload.volumeScatter = inside;
            }
            else // Refract
            {
                payload.rayDirection = Refract(-payload.rayDirection, nrmWorld, 1.0f, IORt);
                payload.hitValue = payload.hitValue * material.i[gl_InstanceCustomIndexEXT].color.rgb;
                payload.volumeScatter = !inside;
            }
            payload.rayOrigin = posWorld + sign(dot(nrmWorld, payload.rayDirection)) * 0.001f * nrmWorld; // Offset a little to avoid self-intersection
        }
        else
        {
            uint state = ApplyVolumeScattering(payload.randomSeed, payload.rayOrigin, payload.rayDirection, payload.hitValue);
            if (state == 0)
            {
                if (random.r < fresnel) // Reflect
                {
                    payload.rayDirection = Reflect(-payload.rayDirection, nrmWorld);
                    payload.hitValue = payload.hitValue;
                    payload.volumeScatter = true;
                }
                else // Refract
                {
                    payload.rayDirection = Refract(-payload.rayDirection, nrmWorld, 1.0f, IORt);
                    payload.hitValue = payload.hitValue;
                    payload.volumeScatter = false;
                }
                payload.rayOrigin = posWorld + sign(dot(nrmWorld, payload.rayDirection)) * 0.001f * nrmWorld; // Offset a little to avoid self-intersection
            }
            else if (state == 1)
            {
                payload.traceEnd = true;
            }
            else
            {
                payload.volumeScatter = true;
            }
        }
    }
    else
    {
        vec3 random = Noise(payload.randomSeed);
        vec3 tangentSample = CosineWeightedSample(random.xy);
        payload.rayDirection = TangentToWorld(nrmWorld, tangentSample);
        // pdf = cos(theta) / pi
        // BRDF = baseColor / pi
        // apply lambert law: BRDF * cos(theta) / pdf = baseColor / pi * cos(theta) / (cos(theta) / pi) = baseColor
        payload.hitValue = payload.hitValue * material.i[gl_InstanceCustomIndexEXT].color.rgb; // Simple diffuse lighting
        payload.rayOrigin = posWorld + sign(dot(nrmWorld, payload.rayDirection)) * 0.001f * nrmWorld; // Offset a little to avoid self-intersection
    }
}