#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "rt_common.glsl"

layout(set = 0, binding = 1) uniform accelerationStructureEXT accelerationStructure;
layout(set = 0, binding = 4) buffer allSpheres_
{
  vec4 allSpheres[];
};
layout(location = 1) rayPayloadInEXT HitPayload payload;
layout(location = 0) rayPayloadEXT bool bHitObject;

// Ray-Sphere intersection
// http://viclw17.github.io/2018/07/16/raytracing-ray-sphere-intersection/
vec3 getSphereNormal(const vec4 s, const vec3 worldPos)
{
  return normalize(worldPos - s.xyz);
}

// Ray-AABB intersection
vec3 getAABBNormal(const vec3 minimum, const vec3 maximum, const vec3 worldPos)
{
  vec3  center = (minimum + maximum) * 0.5;
  vec3  extents = maximum - center;
  vec3  d = worldPos - center;
  vec3  absD = abs(d);
  if(absD.x > absD.y && absD.x > absD.z)
  {
    return vec3(sign(d.x), 0.0, 0.0);
  }
  else if(absD.y > absD.x && absD.y > absD.z)
  {
    return vec3(0.0, sign(d.y), 0.0);
  }
  else
  {
    return vec3(0.0, 0.0, sign(d.z));
  }
}

void EmitShadowRay(in vec3 rayOrigin, in vec3 rayDirection)
{
    uint rayFlags = gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT;
    // | gl_RayFlagsCullBackFacingTrianglesEXT; has artifact at borders if this is enabled
    traceRayEXT(
        accelerationStructure,
        rayFlags,                   // Ray flags
        0xFF,                       // object mask
        0,                          // sbtRecordOffset, which hit shader to call in this Hit Shader group
        0,                          // sbtRecordStride, used in cases where there are multiple objects in one BLAS
        1,                          // missIndex
        rayOrigin,                  // ray origin
        0.001f,                     // ray min range
        rayDirection,               // ray direction
        10000.0f,                   // ray max range
        0                           // payload (location = 1) rayPayloadInEXT can also be used here
    );   
}

const vec3 posLight = vec3(2.0f, 1.0f, 50.0f); // Position of the light source

void main()
{
  // Sphere data
  vec4 sphere = allSpheres[gl_PrimitiveID];
  vec3 worldPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
  vec3 worldNormal = vec3(0.0);
  bHitObject = true;
  if(gl_HitKindEXT == 0)
  {
    // Sphere intersection
    worldNormal = getSphereNormal(sphere, worldPos);
  }
  else
  {
    // AABB intersection
    vec3 aabbMin = sphere.xyz - vec3(sphere.w);
    vec3 aabbMax = sphere.xyz + vec3(sphere.w);
    worldNormal = getAABBNormal(aabbMin, aabbMax, worldPos);
  }

  vec3 wi = normalize(posLight - worldPos);
  payload.hitValue = vec3(clamp(dot(wi, worldNormal), shadowValue, 1.0f));
  vec3 posShadowRay = worldPos + worldNormal * 0.01f; // Offset the shadow ray origin slightly to avoid self-intersection
  EmitShadowRay(posShadowRay, wi);
  if (bHitObject) payload.hitValue = vec3(shadowValue, shadowValue, shadowValue);
}