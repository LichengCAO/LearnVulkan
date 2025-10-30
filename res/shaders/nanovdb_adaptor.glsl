// TO USE THIS DO:
//   1. Add a storage buffer with the same attribute like following:
//     buffer CompactData
//     {
//         uint pnanovdb_buf_data[];
//     }
//     the buffer declaration should be ahead of including this file.
//   2. Call void InitAccessor(uint _rootOffset) in main()

#ifndef NANOVDB_ADAPTOR
#define NANOVDB_ADAPTOR

#define PNANOVDB_GLSL
#include "PNanoVDB.h"

#define GRID_TYPE 1 // TODO

pnanovdb_buf_t          g_unusedBuffer;      // unused in glsl, ignore it
pnanovdb_readaccessor_t g_accessor;          // need to be initialized in main

void InitAccessor(uint _rootOffset)
{
    pnanovdb_root_handle_t rootHandle;
    rootHandle.address = _rootOffset;
    pnanovdb_readaccessor_init(g_accessor, rootHandle);
}

vec3 WorldToIndex(in vec3 _position)
{
    pnanovdb_grid_handle_t gridHandle;
    gridHandle.address = 0u; // grid is the first data in memory layout
    
    return pnanovdb_grid_world_to_indexf(g_unusedBuffer, gridHandle, _position);
}

vec3 IndexToWorld(in vec3 _position)
{
    pnanovdb_grid_handle_t gridHandle;
    gridHandle.address = 0u; // grid is the first data in memory layout

    return pnanovdb_grid_index_to_worldf(g_unusedBuffer, gridHandle, _position);
}

// result is not normalized
vec3 WorldToIndexDirection(in vec3 _direction)
{
    pnanovdb_grid_handle_t gridHandle;
    gridHandle.address = 0u; // grid is the first data in memory layout

    return pnanovdb_grid_world_to_index_dirf(g_unusedBuffer, gridHandle, _direction);
}

// result is not normalized
vec3 IndexToWorldDirection(in vec3 _direction)
{
    pnanovdb_grid_handle_t gridHandle;
    gridHandle.address = 0u; // grid is the first data in memory layout

    return pnanovdb_grid_index_to_world_dirf(g_unusedBuffer, gridHandle, _direction);
}

bool IsActive(in ivec3 _ijk)
{
    return pnanovdb_readaccessor_is_active(GRID_TYPE, g_unusedBuffer, g_accessor, _ijk);
}

float ReadFloat(in ivec3 _ijk)
{
    pnanovdb_address_t address = 
        pnanovdb_readaccessor_get_value_address(
            GRID_TYPE, 
            g_unusedBuffer, 
            g_accessor, 
            _ijk);
    
    return pnanovdb_read_float(g_unusedBuffer, address);
}

bool HDDAZeroCrossing(
    in vec3 _origin, 
    in vec3 _direction,
    inout float _tHit,
    inout float _value)
{
    return pnanovdb_hdda_zero_crossing(
        GRID_TYPE,
        g_unusedBuffer,
        g_accessor,
        _origin,
        0.0001f,
        _direction,
        1000.0f,
        _tHit,
        _value);
}

#endif //NANOVDB_ADAPTOR