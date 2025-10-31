#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "rt_common.glsl"
#include "pbr_common.glsl"
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
  vec3 worldPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
  ivec3 ijk = NanoVDB_IndexToIJK(NanoVDB_WorldToIndex(worldPos));

  if (NanoVDB_IsActive(ijk))
  {
    float value = NanoVDB_ReadFloat(ijk);
    payload.traceEnd = true;
    payload.hitValue = vec3(0, 0, value);
  }
  payload.traceEnd = true;
  payload.hitValue = vec3(0, 0, 1);
}