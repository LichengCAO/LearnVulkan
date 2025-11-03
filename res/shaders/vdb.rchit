#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "rt_common.glsl"
#include "pbr_common.glsl"
#include "pbr_volume_scattering.glsl"
layout(set = 1, binding = 1) buffer nanovdb_
{
  uint rootOffset;
  uint pnanovdb_buf_data[];
};

#include "nanovdb_adaptor.glsl"

layout(location = 1) rayPayloadInEXT PBRPayload payload;

void main()
{
  NanoVDB_Init(rootOffset);
  vec3 throughPut = vec3(1.0f);
  vec3 indexPos = NanoVDB_WorldToIndex(gl_WorldRayOriginEXT);
  vec3 indexDir = NanoVDB_WorldToIndexDirection(gl_ObjectRayDirectionEXT);
  vec3 indexMinBBox;
  vec3 indexMaxBBox;
  uint state = 0;

  NanoVDB_GetIndexBoundingBox(indexMinBBox, indexMaxBBox);
  for (int i = 0; i < 8; ++i)
  {
    float density = 0.0f;
    ivec3 ijk = NanoVDB_IndexToIJK(indexPos);
    float tMin = 0.001f;
    float tMax = 1000.0f;
    if (NanoVDB_IsActive(ijk))
    {
      density = NanoVDB_ReadFloat(ijk);
    }
    NanoVDB_HDDARayClip(
      indexMinBBox,
      indexMaxBBox,
      indexPos,
      indexDir,
      tMin,
      tMax);
    SampleVolume(        
      pcg3d(uvec3(gl_LaunchIDEXT.xy, i)),
      pcg3d_2(uvec3(gl_LaunchIDEXT.xy, i)),
      tMin,
      0,
      0.5 * density,
      0.5 * density,
      5.0f - density,
      throughPut,
      indexPos,
      indexDir,
      state);
    if (state == 0)
    {
      throughPut *= (gl_ObjectRayDirectionEXT * 0.5 + vec3(0.5f));
      break;
    }
    else if (state == 1)
    {
      break;
    }
  }
  
  payload.traceEnd = true;
  payload.hitValue = throughPut;
}