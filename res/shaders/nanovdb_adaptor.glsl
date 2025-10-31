// TO USE THIS DO:
//   1. Add a storage buffer with the same attribute like following:
//     buffer CompactData
//     {
//         uint pnanovdb_buf_data[];
//     };
//     the buffer declaration should be ahead of including this file.
//   2. Call void NanoVDB_InitAccessor(uint _rootOffset) in main()

#ifndef NANOVDB_ADAPTOR
#define NANOVDB_ADAPTOR

#define PNANOVDB_GLSL
#include "PNanoVDB.h"

#define GRID_FLOAT_TYPE 1 // See NanoVDB.h

pnanovdb_buf_t          g_unusedBuffer;      // unused in glsl, ignore it
pnanovdb_readaccessor_t g_accessor;          // need to be initialized in main

pnanovdb_address_t _CreateNanoVDBAddress(uint _offset)
{
    pnanovdb_address_t offset;
    offset.byte_offset = _offset;
    return offset;
}

void NanoVDB_InitAccessor(uint _rootOffset)
{
    pnanovdb_root_handle_t rootHandle;
    rootHandle.address = _CreateNanoVDBAddress(_rootOffset);
    pnanovdb_readaccessor_init(g_accessor, rootHandle);
}

vec3 NanoVDB_WorldToIndex(in vec3 _position)
{
    pnanovdb_grid_handle_t gridHandle;
    gridHandle.address = _CreateNanoVDBAddress(0); // grid is the first data in memory layout
    
    return pnanovdb_grid_world_to_indexf(g_unusedBuffer, gridHandle, _position);
}

vec3 NanoVDB_IndexToWorld(in vec3 _position)
{
    pnanovdb_grid_handle_t gridHandle;
    gridHandle.address = _CreateNanoVDBAddress(0); // grid is the first data in memory layout

    return pnanovdb_grid_index_to_worldf(g_unusedBuffer, gridHandle, _position);
}

// result is not normalized
vec3 NanoVDB_WorldToIndexDirection(in vec3 _direction)
{
    pnanovdb_grid_handle_t gridHandle;
    gridHandle.address = _CreateNanoVDBAddress(0); // grid is the first data in memory layout

    return pnanovdb_grid_world_to_index_dirf(g_unusedBuffer, gridHandle, _direction);
}

// result is not normalized
vec3 NanoVDB_IndexToWorldDirection(in vec3 _direction)
{
    pnanovdb_grid_handle_t gridHandle;
    gridHandle.address = _CreateNanoVDBAddress(0); // grid is the first data in memory layout

    return pnanovdb_grid_index_to_world_dirf(g_unusedBuffer, gridHandle, _direction);
}

bool NanoVDB_IsActive(in ivec3 _ijk)
{
    pnanovdb_grid_handle_t gridHandle;
    gridHandle.address = _CreateNanoVDBAddress(0); // grid is the first data in memory layout
    
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

#endif //NANOVDB_ADAPTOR