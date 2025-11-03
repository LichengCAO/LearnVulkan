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
  float majorant;
  uint pnanovdb_buf_data[];
};

#include "nanovdb_adaptor.glsl"

layout(location = 1) rayPayloadInEXT PBRPayload payload;
layout(push_constant) uniform shaderInformation
{
  layout(offset = 4)float single_scatter_albedo;
  float density;
} mediumInfo;

void main()
{
  NanoVDB_Init(rootOffset);
  vec3 throughPut = vec3(1.0f);
  vec3 indexPos = NanoVDB_WorldToIndex(gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT);
  vec3 indexDir = NanoVDB_WorldToIndexDirection(gl_WorldRayDirectionEXT);
  vec3 indexMinBBox;
  vec3 indexMaxBBox;
  int i = 0;
  bool fogged = false;

  NanoVDB_GetIndexBoundingBox(indexMinBBox, indexMaxBBox);
  while(i < 800)
  {
    float t;

    if (!NanoVDB_HitAABB(indexMinBBox, indexMaxBBox, indexPos, indexDir, t))
    {
      break;
    }
    float t1 = SampleExponential(pcg3d(uvec3(gl_LaunchIDEXT.xy, payload.randomSeed + i)).x, majorant);

    // Lo
    if (t1 > t)
    {
      throughPut *= (normalize(NanoVDB_IndexToWorldDirection(indexDir)) * 0.5 + vec3(0.5f));
      break;
    }

    // else, Ln
    // update ray origin
    indexPos = t1 * indexDir + indexPos;
    
    float sigma_a = 0.0f;
    float sigma_s = 0.0f;
    float sigma_n = majorant;
    float g = 0.0f;
    ivec3 ijk = NanoVDB_IndexToIJK(indexPos);
    bool absorbed;
    if (NanoVDB_IsActive(ijk))
    {
      // sample medium properties
      float sigma_t = mediumInfo.density * NanoVDB_ReadFloat(ijk);
      sigma_s = mediumInfo.single_scatter_albedo * sigma_t;
      sigma_a = sigma_t - sigma_s;
      sigma_n = majorant - sigma_t;
      // update g maybe
    }
    uint state;
    Ln(pcg3d_2(uvec3(gl_LaunchIDEXT.xy, payload.randomSeed + i)), g, sigma_a, sigma_s, sigma_n, indexDir, throughPut, state);
    if (state == 0)
    {
      fogged = true;
      break;
    }
    else if (state == 1)
    {
      fogged = true;
    }

    i++;
  }
  if (!fogged)
  {
    throughPut = vec3(0.0);
  }
  payload.traceEnd = true;
  payload.hitValue = throughPut;
}