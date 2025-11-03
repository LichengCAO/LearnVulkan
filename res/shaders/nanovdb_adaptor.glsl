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
pnanovdb_grid_handle_t  g_gridHandle;

pnanovdb_address_t _CreateNanoVDBAddress(uint _offset)
{
    pnanovdb_address_t offset;
    offset.byte_offset = _offset;
    return offset;
}

void NanoVDB_Init(uint _rootOffset)
{
    pnanovdb_root_handle_t rootHandle;
    rootHandle.address = _CreateNanoVDBAddress(_rootOffset);

    pnanovdb_readaccessor_init(g_accessor, rootHandle);

    g_gridHandle.address = _CreateNanoVDBAddress(0); // grid is the first data in memory layout
}

vec3 NanoVDB_WorldToIndex(in vec3 _position)
{    
    return pnanovdb_grid_world_to_indexf(g_unusedBuffer, g_gridHandle, _position);
}

vec3 NanoVDB_IndexToWorld(in vec3 _position)
{
    return pnanovdb_grid_index_to_worldf(g_unusedBuffer, g_gridHandle, _position);
}

// result is not normalized
vec3 NanoVDB_WorldToIndexDirection(in vec3 _direction)
{
    return pnanovdb_grid_world_to_index_dirf(g_unusedBuffer, g_gridHandle, _direction);
}

// result is not normalized
vec3 NanoVDB_IndexToWorldDirection(in vec3 _direction)
{
    return pnanovdb_grid_index_to_world_dirf(g_unusedBuffer, g_gridHandle, _direction);
}

bool NanoVDB_IsActive(in ivec3 _ijk)
{
    uint type = pnanovdb_grid_get_grid_type(g_unusedBuffer, g_gridHandle);
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

void NanoVDB_GetGridWorldBoundingBox(out vec3 _min, out vec3 _max)
{
    _min = vec3(
        pnanovdb_grid_get_world_bbox(g_unusedBuffer, g_gridHandle, 0),
        pnanovdb_grid_get_world_bbox(g_unusedBuffer, g_gridHandle, 1),
        pnanovdb_grid_get_world_bbox(g_unusedBuffer, g_gridHandle, 2));
    _max = vec3(
        pnanovdb_grid_get_world_bbox(g_unusedBuffer, g_gridHandle, 3),
        pnanovdb_grid_get_world_bbox(g_unusedBuffer, g_gridHandle, 4),
        pnanovdb_grid_get_world_bbox(g_unusedBuffer, g_gridHandle, 5));
}

void NanoVDB_GetIndexBoundingBox(out vec3 _min, out vec3 _max)
{
    ivec3 bbox_min = pnanovdb_root_get_bbox_min(g_unusedBuffer, g_accessor.root);
    ivec3 bbox_max = pnanovdb_root_get_bbox_max(g_unusedBuffer, g_accessor.root);
    _min = pnanovdb_coord_to_vec3(bbox_min);
    _max = pnanovdb_coord_to_vec3(pnanovdb_coord_add(bbox_max, pnanovdb_coord_uniform(1)));
}

// return true if the ray transfer from + to - or - to +
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

// return true if the ray intersect with the domain
bool NanoVDB_HDDARayClip(
    in vec3 _minBBox,
    in vec3 _maxBBox,
    in vec3 _origin,
    in vec3 _direction,
    inout float _tMin,
    inout float _tFar)
{
    return pnanovdb_hdda_ray_clip(_minBBox, _maxBBox, _origin, _tMin, _direction, _tFar);
}

#endif //NANOVDB_ADAPTOR