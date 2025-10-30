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
} _nanovdb;
#define PNANOVDB_GLSL
#include "PNanoVDB.h"

layout(location = 1) rayPayloadInEXT PBRPayload payload;

//==================================================================================

#define GRID_FLOAT_TYPE 1 // See NanoVDB.h

pnanovdb_buf_t          g_unusedBuffer;      // unused in glsl, ignore it
pnanovdb_readaccessor_t g_accessor;          // need to be initialized in main

void NanoVDB_InitAccessor(uint _rootOffset)
{
    pnanovdb_root_handle_t rootHandle;
    rootHandle.address = _rootOffset;
    pnanovdb_readaccessor_init(g_accessor, rootHandle);
}

vec3 NanoVDB_WorldToIndex(in vec3 _position)
{
    pnanovdb_grid_handle_t gridHandle;
    gridHandle.address = 0u; // grid is the first data in memory layout
    
    return pnanovdb_grid_world_to_indexf(g_unusedBuffer, gridHandle, _position);
}

vec3 NanoVDB_IndexToWorld(in vec3 _position)
{
    pnanovdb_grid_handle_t gridHandle;
    gridHandle.address = 0u; // grid is the first data in memory layout

    return pnanovdb_grid_index_to_worldf(g_unusedBuffer, gridHandle, _position);
}

// result is not normalized
vec3 NanoVDB_WorldToIndexDirection(in vec3 _direction)
{
    pnanovdb_grid_handle_t gridHandle;
    gridHandle.address = 0u; // grid is the first data in memory layout

    return pnanovdb_grid_world_to_index_dirf(g_unusedBuffer, gridHandle, _direction);
}

// result is not normalized
vec3 NanoVDB_IndexToWorldDirection(in vec3 _direction)
{
    pnanovdb_grid_handle_t gridHandle;
    gridHandle.address = 0u; // grid is the first data in memory layout

    return pnanovdb_grid_index_to_world_dirf(g_unusedBuffer, gridHandle, _direction);
}

bool NanoVDB_IsActive(in ivec3 _ijk)
{
    pnanovdb_grid_handle_t gridHandle;
    gridHandle.address = 0u; // grid is the first data in memory layout
    
    uint type = pnanovdb_grid_get_grid_type(g_unusedBuffer, gridHandle);
    return pnanovdb_readaccessor_is_active(type, g_unusedBuffer, g_accessor, _ijk);
}

float NanoVDB_ReadFloat(in ivec3 _ijk)
{
    pnanovdb_address_t address = 
        pnanovdb_readaccessor_get_value_address(
            GRID_FLOAT_TYPE, 
            g_unusedBuffer, 
            g_accessor, 
            _ijk);
    
    return pnanovdb_read_float(g_unusedBuffer, address);
}

ivec3 NanoVDB_IndexToIJK(in vec3 _indexPosition)
{
    return ivec3(_indexPosition);
}

// _origin: index space
// _direction: index space
bool NanoVDB_HDDAZeroCrossing(
    in vec3 _origin, 
    in vec3 _direction,
    inout float _tHit,
    inout float _value)
{
    return pnanovdb_hdda_zero_crossing(
        GRID_FLOAT_TYPE,
        g_unusedBuffer,
        g_accessor,
        _origin,
        0.0001f,
        _direction,
        1000.0f,
        _tHit,
        _value);
}

//==================================================================================


void main()
{
  NanoVDB_InitAccessor(rootOffset);
  vec3 worldPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
  vec3 ijk = NanoVDB_IndexToIJK(NanoVDB_WorldToIndex(worldPos));

  if (NanoVDB_IsActive(ijk))
  {
    float value = NanoVDB_ReadFloat(NanoVDB_IndexToIJK(ijk));
    payload.traceEnd = true;
    payload.hitValue = vec3(0, 0, value);
  }
  
}