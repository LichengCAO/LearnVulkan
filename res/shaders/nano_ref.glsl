
#pragma optimize(on)
struct HDDA
{
    int mDim;
    float mT0, mT1;
    ivec3 mVoxel, mStep;
    vec3 mDelta, mNext;
};
const float DeltaFloat = 0.0001f;
const float MaxFloat = 9999999.f;
const int MinIndex_hashTable[8] = {2, 1, 9, 1, 2, 9, 0, 0};
int MinIndex(vec3 v)
{
    return (v.y < v.x) ? (v.z < v.y ? 2 : 1) : (v.z < v.x ? 2 : 0);
}
struct nanovdb_Coord
{
    int mVec[3];
};
struct nanovdb_Map
{
    float mMatF[9];
    float mInvMatF[9];
    float mVecF[3];
    float mTaperF;
    double mMatD[9];
    double mInvMatD[9];
    double mVecD[3];
    double mTaperD;
};
struct nanovdb_Ray
{
    vec3 mEye;
    vec3 mDir;
    float mT0;
    float mT1;
};
struct nanovdb_BitMask3
{
    uint mWords[(1 << (3 * 3)) >> 5];
};
;
struct nanovdb_BitMask4
{
    uint mWords[(1 << (3 * 4)) >> 5];
};
;
struct nanovdb_BitMask5
{
    uint mWords[(1 << (3 * 5)) >> 5];
};
;
struct nanovdb_ReadAccessor
{
    nanovdb_Coord mKey;
    int mNodeIndex[3];
};
void nanovdb_ReadAccessor_insert(inout nanovdb_ReadAccessor acc, int childlevel, int nodeIndex, nanovdb_Coord ijk)
{
    acc.mNodeIndex[childlevel] = nodeIndex;
    acc.mKey.mVec[0] = ijk.mVec[0];
    acc.mKey.mVec[1] = ijk.mVec[1];
    acc.mKey.mVec[2] = ijk.mVec[2];
}
uvec2 nanovdb_coord_to_key(nanovdb_Coord ijk)
{
    uvec2 key = uvec2((uint(ijk.mVec[2])) >> 12, 0) |
                uvec2(((uint(ijk.mVec[1])) >> 12) << 21,
                      (uint(ijk.mVec[1])) >> 23) |
                uvec2(0, ((uint(ijk.mVec[0])) >> 12) << 10);
    return key;
}
struct nanovdb_GridData
{
    uvec2 mMagic;
    uvec2 mChecksum;
    uint mMajor;
    uint mFlags;
    uvec2 mGridSize;
    uint mGridName[256 / 4];
    nanovdb_Map mMap;
    double mBBoxMin[3];
    double mBBoxMax[3];
    double mVoxelSize[3];
    uint mGridClass;
    uint mGridType;
    uvec2 mBlindMetadataOffset;
    uint mBlindMetadataCount;
    uint _reserved[(-(8 + 8 + 4 + 4 + 8 + 256 + 24 + 24 + (4 * 9 + 4 * 9 + 4 * 3 + 4 + 8 * 9 + 8 * 9 + 8 * 3 + 8) + 24 + 4 + 4 + 8 + 4) & ((32) - 1)) / 4];
};
struct nanovdb_GridBlindMetaData
{
    ivec2 mByteOffset;
    ivec2 mByteSize;
    uint mFlags;
    uint mSemantic;
    uint mDataClass;
    uint mDataType;
    uint mName[256 / 4];
};
struct nanovdb_Node0_float
{
    nanovdb_Coord mBBox_min;
    uint mBBoxDifAndFlags;
    nanovdb_BitMask3 mValueMask;
    float mValueMin;
    float mValueMax;
    uint _reserved[(-((8 << (3 * 3 - 6)) + (12) + (4) + (4 * 2)) & ((32) - 1)) / 4];
    float mVoxels[1 << (3 * 3)];
};
const int nanovdb_Node0_TOTAL = 3;
const int nanovdb_Node0_MASK = ((1 << 3) - 1);
struct nanovdb_TileEntry_float
{
    int valueOrChildID;
};
struct nanovdb_Node1_float
{
    nanovdb_Coord mBBox_min, mBBox_max;
    int mOffset;
    uint mFlags;
    nanovdb_BitMask4 mValueMask;
    nanovdb_BitMask4 mChildMask;
    float mValueMin;
    float mValueMax;
    uint _reserved[(-((24) + (4) + (4) + (8 << (3 * 4 - 6)) * 2 + (4 * 2)) & ((32) - 1)) / 4];
    nanovdb_TileEntry_float mTable[1 << (3 * 4)];
};
const int nanovdb_Node1_TOTAL = 7;
const int nanovdb_Node1_MASK = ((1 << 7) - 1);

struct nanovdb_Node2_float
{
    nanovdb_Coord mBBox_min, mBBox_max;
    int mOffset;
    uint mFlags;
    nanovdb_BitMask5 mValueMask;
    nanovdb_BitMask5 mChildMask;
    float mValueMin;
    float mValueMax;
    uint _reserved[(-((24) + (4) + (4) + (8 << (3 * 5 - 6)) * 2 + (4 * 2)) & ((32) - 1)) / 4];
    nanovdb_TileEntry_float mTable[1 << (3 * 5)];
};
const int nanovdb_Node2_TOTAL = 12;
const int nanovdb_Node2_MASK = ((1 << 12) - 1);

struct nanovdb_RootData_Tile_float
{
    uvec2 key;
    int childID;
    uint state;
    float value;
    uint _reserved[(-((8) + (4) + (4) + (4)) & ((32) - 1)) / 4];
};

struct nanovdb_RootData_float
{
    nanovdb_Coord mBBox_min, mBBox_max;
    uvec2 mActiveVoxelCount;
    int mTileCount;
    float mBackground, mValueMin, mValueMax;
    uint _reserved[(-((24) + (8) + (4) + (4 * 3)) & ((32) - 1)) / 4];
};

layout(std430, binding = 0) restrict readonly buffer nanovdb_Node0_Block
{
    nanovdb_Node0_float nodes[];
}
kNodeLevel0;
layout(std430, binding = 1) restrict readonly buffer nanovdb_Node1_Block
{
    nanovdb_Node1_float nodes[];
}
kNodeLevel1;
layout(std430, binding = 2) restrict readonly buffer nanovdb_Node2_Block
{
    nanovdb_Node2_float nodes[];
}
kNodeLevel2;
layout(std430, binding = 3) readonly buffer nanovdb_Root_Block
{
    nanovdb_RootData_float root;
    nanovdb_RootData_Tile_float tiles[];
}
kRootData;
layout(std430, binding = 4) readonly buffer nanovdb_Grid_Block
{
    nanovdb_GridData grid;
}
kGridData;
layout(rgba32f, binding = 0) uniform image2D kOutImage;
vec3 nanovdb_CoordToVec3f(const nanovdb_Coord x)
{
    return vec3(float(float(x.mVec[0])), float(float(x.mVec[1])), float(float(x.mVec[2])));
}
nanovdb_Coord nanovdb_Vec3fToCoord(const vec3 p)
{
    nanovdb_Coord x;
    x.mVec[0] = int(floor(p.x));
    x.mVec[1] = int(floor(p.y));
    x.mVec[2] = int(floor(p.z));
    return x;
}
nanovdb_Coord nanovdb_Vec3iToCoord(const ivec3 p)
{
    nanovdb_Coord x;
    x.mVec[0] = p.x;
    x.mVec[1] = p.y;
    x.mVec[2] = p.z;
    return x;
}
bool nanovdb_Ray_clip(inout nanovdb_Ray ray, vec3 p0, vec3 p1)
{
    vec3 t0 = ((((p0) - (ray.mEye))) / (ray.mDir));
    vec3 t1 = ((((p1) - (ray.mEye))) / (ray.mDir));
    vec3 tmin = vec3(float(min(t0.x, t1.x)), float(min(t0.y, t1.y)), float(min(t0.z, t1.z)));
    vec3 tmax = vec3(float(max(t0.x, t1.x)), float(max(t0.y, t1.y)), float(max(t0.z, t1.z)));
    float mint = max(tmin.x, max(tmin.y, tmin.z));
    float maxt = min(tmax.x, min(tmax.y, tmax.z));
    bool hit = (mint <= maxt);
    ray.mT0 = max(ray.mT0, mint);
    ray.mT1 = min(ray.mT1, maxt);
    return hit;
}
vec3 nanovdb_Ray_start(inout nanovdb_Ray ray)
{
    return ((ray.mEye) + (((ray.mT0) * (ray.mDir))));
}
vec3 nanovdb_Ray_eval(inout nanovdb_Ray ray, float t)
{
    return ((ray.mEye) + (((t) * (ray.mDir))));
}
vec3 nanovdb_Map_apply(nanovdb_Map map, const vec3 src)
{
    vec3 dst;
    float sx = src.x;
    float sy = src.y;
    float sz = src.z;
    dst.x = sx * map.mMatF[0] + sy * map.mMatF[1] + sz * map.mMatF[2] + map.mVecF[0];
    dst.y = sx * map.mMatF[3] + sy * map.mMatF[4] + sz * map.mMatF[5] + map.mVecF[1];
    dst.z = sx * map.mMatF[6] + sy * map.mMatF[7] + sz * map.mMatF[8] + map.mVecF[2];
    return dst;
}
vec3 nanovdb_Map_applyInverse(nanovdb_Map map, const vec3 src)
{
    vec3 dst;
    float sx = src.x - map.mVecF[0];
    float sy = src.y - map.mVecF[1];
    float sz = src.z - map.mVecF[2];
    dst.x = sx * map.mInvMatF[0] + sy * map.mInvMatF[1] + sz * map.mInvMatF[2];
    dst.y = sx * map.mInvMatF[3] + sy * map.mInvMatF[4] + sz * map.mInvMatF[5];
    dst.z = sx * map.mInvMatF[6] + sy * map.mInvMatF[7] + sz * map.mInvMatF[8];
    return dst;
}
vec3 nanovdb_Map_applyJacobi(nanovdb_Map map, const vec3 src)
{
    vec3 dst;
    float sx = src.x;
    float sy = src.y;
    float sz = src.z;
    dst.x = sx * map.mMatF[0] + sy * map.mMatF[1] + sz * map.mMatF[2];
    dst.y = sx * map.mMatF[3] + sy * map.mMatF[4] + sz * map.mMatF[5];
    dst.z = sx * map.mMatF[6] + sy * map.mMatF[7] + sz * map.mMatF[8];
    return dst;
}
vec3 nanovdb_Map_applyInverseJacobi(nanovdb_Map map, const vec3 src)
{
    vec3 dst;
    float sx = src.x;
    float sy = src.y;
    float sz = src.z;
    dst.x = sx * map.mInvMatF[0] + sy * map.mInvMatF[1] + sz * map.mInvMatF[2];
    dst.y = sx * map.mInvMatF[3] + sy * map.mInvMatF[4] + sz * map.mInvMatF[5];
    dst.z = sx * map.mInvMatF[6] + sy * map.mInvMatF[7] + sz * map.mInvMatF[8];
    return dst;
}
int nanovdb_Node0_float_CoordToOffset(nanovdb_Coord ijk) 
{ 
    return (((ijk.mVec[0] & ((1 << 3) - 1)) >> (3 - 3)) << (2 * 3)) + (((ijk.mVec[1] & ((1 << 3) - 1)) >> (3 - 3)) << (3)) + ((ijk.mVec[2] & ((1 << 3) - 1)) >> (3 - 3)); 
}
float nanovdb_Node0_float_getValue(int cxt, int nodeIndex, nanovdb_Coord ijk)
{
    int n = nanovdb_Node0_float_CoordToOffset(ijk);
    return kNodeLevel0.nodes[nodeIndex].mVoxels[n];
}
float nanovdb_Node0_float_getValueAndCache(int cxt, int nodeIndex, nanovdb_Coord ijk, inout nanovdb_ReadAccessor acc)
{
    int n = nanovdb_Node0_float_CoordToOffset(ijk);
    return kNodeLevel0.nodes[nodeIndex].mVoxels[n];
}
int nanovdb_Node0_float_getDimAndCache(int cxt, int nodeIndex, nanovdb_Coord ijk, nanovdb_Ray ray, inout nanovdb_ReadAccessor acc) 
{ 
    return ((kNodeLevel0.nodes[nodeIndex].mBBoxDifAndFlags & 7) != 0u) ? 1 << 3 : 1; 
};
float nanovdb_TileEntry_float_getValue(nanovdb_TileEntry_float entry) 
{ 
    return intBitsToFloat(entry.valueOrChildID); 
}
int nanovdb_TileEntry_float_getChild(nanovdb_TileEntry_float entry) 
{ 
    return entry.valueOrChildID; 
};
int nanovdb_Node1_float_CoordToOffset(nanovdb_Coord ijk) 
{ 
    return (((ijk.mVec[0] & ((1 << 7) - 1)) >> nanovdb_Node0_TOTAL) << (2 * 4)) + (((ijk.mVec[1] & ((1 << 7) - 1)) >> nanovdb_Node0_TOTAL) << (4)) + ((ijk.mVec[2] & ((1 << 7) - 1)) >> nanovdb_Node0_TOTAL); 
}
int nanovdb_Node1_float_getChild(int cxt, int nodeIndex, int n) 
{ 
    return nanovdb_TileEntry_float_getChild(kNodeLevel1.nodes[nodeIndex].mTable[n]); 
}
bool nanovdb_Node1_float_hasChild(int cxt, int nodeIndex, int n) 
{ 
    return 0 != (kNodeLevel1.nodes[nodeIndex].mChildMask.mWords[n >> 5] & (uint(1) << (n & 31))); 
}
float nanovdb_Node1_float_getValue(int cxt, int nodeIndex, nanovdb_Coord ijk)
{
    int n = nanovdb_Node1_float_CoordToOffset(ijk);
    if (nanovdb_Node1_float_hasChild(cxt, nodeIndex, n))
    {
        int childIndex = nanovdb_Node1_float_getChild(cxt, nodeIndex, n);
        return nanovdb_Node0_float_getValue(cxt, childIndex, ijk);
    }
    return nanovdb_TileEntry_float_getValue(kNodeLevel1.nodes[nodeIndex].mTable[n]);
}
float nanovdb_Node1_float_getValueAndCache(int cxt, int nodeIndex, nanovdb_Coord ijk, inout nanovdb_ReadAccessor acc)
{
    int n = nanovdb_Node1_float_CoordToOffset(ijk);
    if (nanovdb_Node1_float_hasChild(cxt, nodeIndex, n))
    {
        int childIndex = nanovdb_Node1_float_getChild(cxt, nodeIndex, n);
        nanovdb_ReadAccessor_insert(acc, 0, childIndex, ijk);
        return nanovdb_Node0_float_getValueAndCache(cxt, childIndex, ijk, acc);
    }
    return nanovdb_TileEntry_float_getValue(kNodeLevel1.nodes[nodeIndex].mTable[n]);
}
int nanovdb_Node1_float_getDimAndCache(int cxt, int nodeIndex, nanovdb_Coord ijk, nanovdb_Ray ray, inout nanovdb_ReadAccessor acc)
{
    int n = nanovdb_Node1_float_CoordToOffset(ijk);
    if (nanovdb_Node1_float_hasChild(cxt, nodeIndex, n))
    {
        int childIndex = nanovdb_Node1_float_getChild(cxt, nodeIndex, n);
        nanovdb_ReadAccessor_insert(acc, 0, childIndex, ijk);
        return nanovdb_Node0_float_getDimAndCache(cxt, childIndex, ijk, ray, acc);
    }
    return 1 << nanovdb_Node0_TOTAL;
};
int nanovdb_Node2_float_CoordToOffset(nanovdb_Coord ijk) 
{ 
    return (((ijk.mVec[0] & ((1 << 12) - 1)) >> nanovdb_Node1_TOTAL) << (2 * 5)) + (((ijk.mVec[1] & ((1 << 12) - 1)) >> nanovdb_Node1_TOTAL) << (5)) + ((ijk.mVec[2] & ((1 << 12) - 1)) >> nanovdb_Node1_TOTAL); 
}
int nanovdb_Node2_float_getChild(int cxt, int nodeIndex, int n) 
{ 
    return nanovdb_TileEntry_float_getChild(kNodeLevel2.nodes[nodeIndex].mTable[n]); 
}
bool nanovdb_Node2_float_hasChild(int cxt, int nodeIndex, int n) 
{ 
    return 0 != (kNodeLevel2.nodes[nodeIndex].mChildMask.mWords[n >> 5] & (uint(1) << (n & 31))); 
}
float nanovdb_Node2_float_getValue(int cxt, int nodeIndex, nanovdb_Coord ijk)
{
    int n = nanovdb_Node2_float_CoordToOffset(ijk);
    if (nanovdb_Node2_float_hasChild(cxt, nodeIndex, n))
    {
        int childIndex = nanovdb_Node2_float_getChild(cxt, nodeIndex, n);
        return nanovdb_Node1_float_getValue(cxt, childIndex, ijk);
    }
    return nanovdb_TileEntry_float_getValue(kNodeLevel2.nodes[nodeIndex].mTable[n]);
}
float nanovdb_Node2_float_getValueAndCache(int cxt, int nodeIndex, nanovdb_Coord ijk, inout nanovdb_ReadAccessor acc)
{
    int n = nanovdb_Node2_float_CoordToOffset(ijk);
    if (nanovdb_Node2_float_hasChild(cxt, nodeIndex, n))
    {
        int childIndex = nanovdb_Node2_float_getChild(cxt, nodeIndex, n);
        nanovdb_ReadAccessor_insert(acc, 1, childIndex, ijk);
        return nanovdb_Node1_float_getValueAndCache(cxt, childIndex, ijk, acc);
    }
    return nanovdb_TileEntry_float_getValue(kNodeLevel2.nodes[nodeIndex].mTable[n]);
}
int nanovdb_Node2_float_getDimAndCache(int cxt, int nodeIndex, nanovdb_Coord ijk, nanovdb_Ray ray, inout nanovdb_ReadAccessor acc)
{
    int n = nanovdb_Node2_float_CoordToOffset(ijk);
    if (nanovdb_Node2_float_hasChild(cxt, nodeIndex, n))
    {
        int childIndex = nanovdb_Node2_float_getChild(cxt, nodeIndex, n);
        nanovdb_ReadAccessor_insert(acc, 1, childIndex, ijk);
        return nanovdb_Node1_float_getDimAndCache(cxt, childIndex, ijk, ray, acc);
    }
    return 1 << nanovdb_Node1_TOTAL;
};
int nanovdb_RootData_float_findTile(int cxt, nanovdb_Coord ijk)
{
    int low = 0, high = kRootData.root.mTileCount;
    uvec2 key = nanovdb_coord_to_key(ijk);
    for (int i = low; i < high; i++)
    {
        if (kRootData.tiles[i].key.x == key.x && kRootData.tiles[i].key.y == key.y)
            return int(i);
    };
    return -1;
}
float nanovdb_RootData_float_getValue(int cxt, nanovdb_Coord ijk)
{
    int rootTileIndex = nanovdb_RootData_float_findTile(cxt, ijk);
    if (rootTileIndex < 0)
        return kRootData.root.mBackground;
    int childIndex = kRootData.tiles[rootTileIndex].childID;
    if (childIndex < 0)
        return kRootData.tiles[rootTileIndex].value;
    return nanovdb_Node2_float_getValue(cxt, childIndex, ijk);
}
float nanovdb_RootData_float_getValueAndCache(int cxt, nanovdb_Coord ijk, inout nanovdb_ReadAccessor acc)
{
    int rootTileIndex = nanovdb_RootData_float_findTile(cxt, ijk);
    if (rootTileIndex < 0)
        return kRootData.root.mBackground;
    int childIndex = kRootData.tiles[rootTileIndex].childID;
    if (childIndex < 0)
        return kRootData.tiles[rootTileIndex].value;
    nanovdb_ReadAccessor_insert(acc, 2, childIndex, ijk);
    return nanovdb_Node2_float_getValueAndCache(cxt, childIndex, ijk, acc);
}
int nanovdb_RootData_float_getDimAndCache(int cxt, nanovdb_Coord ijk, nanovdb_Ray ray, inout nanovdb_ReadAccessor acc)
{
    int rootTileIndex = nanovdb_RootData_float_findTile(cxt, ijk);
    if (rootTileIndex < 0)
        return 1 << nanovdb_Node2_TOTAL;
    int childIndex = kRootData.tiles[rootTileIndex].childID;
    if (childIndex < 0)
        return 1 << nanovdb_Node2_TOTAL;
    nanovdb_ReadAccessor_insert(acc, 2, childIndex, ijk);
    return nanovdb_Node2_float_getDimAndCache(cxt, childIndex, ijk, ray, acc);
};
nanovdb_ReadAccessor
nanovdb_ReadAccessor_create()
{
    nanovdb_ReadAccessor acc;
    acc.mKey.mVec[0] = acc.mKey.mVec[1] = acc.mKey.mVec[2] = 0;
    acc.mNodeIndex[0] = acc.mNodeIndex[1] = acc.mNodeIndex[2] = -1;
    return acc;
}
bool nanovdb_ReadAccessor_isCached0(inout nanovdb_ReadAccessor acc, int dirty)
{
    if (acc.mNodeIndex[0] < 0)
        return false;
    if ((dirty & ~((1u << 3) - 1u)) != 0)
    {
        acc.mNodeIndex[0] = -1;
        return false;
    }
    return true;
}
bool nanovdb_ReadAccessor_isCached1(inout nanovdb_ReadAccessor acc, int dirty)
{
    if (acc.mNodeIndex[1] < 0)
        return false;
    if ((dirty & ~((1u << 7) - 1u)) != 0)
    {
        acc.mNodeIndex[1] = -1;
        return false;
    }
    return true;
}
bool nanovdb_ReadAccessor_isCached2(inout nanovdb_ReadAccessor acc, int dirty)
{
    if (acc.mNodeIndex[2] < 0)
        return false;
    if ((dirty & ~((1u << 12) - 1u)) != 0)
    {
        acc.mNodeIndex[2] = -1;
        return false;
    }
    return true;
}
int nanovdb_ReadAccessor_computeDirty(nanovdb_ReadAccessor acc, nanovdb_Coord ijk)
{
    return (ijk.mVec[0] ^ acc.mKey.mVec[0]) |
           (ijk.mVec[1] ^ acc.mKey.mVec[1]) |
           (ijk.mVec[2] ^ acc.mKey.mVec[2]);
}
float nanovdb_ReadAccessor_getValue(int cxt, inout nanovdb_ReadAccessor acc, nanovdb_Coord ijk)
{
    int dirty = nanovdb_ReadAccessor_computeDirty(acc, ijk);
    if (nanovdb_ReadAccessor_isCached0(acc, dirty))
        return nanovdb_Node0_float_getValueAndCache(cxt, acc.mNodeIndex[0], ijk, acc);
    else if (nanovdb_ReadAccessor_isCached1(acc, dirty))
        return nanovdb_Node1_float_getValueAndCache(cxt, acc.mNodeIndex[1], ijk, acc);
    else if (nanovdb_ReadAccessor_isCached2(acc, dirty))
        return nanovdb_Node2_float_getValueAndCache(cxt, acc.mNodeIndex[2], ijk, acc);
    else
        return nanovdb_RootData_float_getValueAndCache(cxt, ijk, acc);
}
int nanovdb_ReadAccessor_getDim(int cxt, inout nanovdb_ReadAccessor acc, nanovdb_Coord ijk, nanovdb_Ray ray)
{
    int dirty = nanovdb_ReadAccessor_computeDirty(acc, ijk);
    if (nanovdb_ReadAccessor_isCached0(acc, dirty))
        return nanovdb_Node0_float_getDimAndCache(cxt, acc.mNodeIndex[0], ijk, ray, acc);
    else if (nanovdb_ReadAccessor_isCached1(acc, dirty))
        return nanovdb_Node1_float_getDimAndCache(cxt, acc.mNodeIndex[1], ijk, ray, acc);
    else if (nanovdb_ReadAccessor_isCached2(acc, dirty))
        return nanovdb_Node2_float_getDimAndCache(cxt, acc.mNodeIndex[2], ijk, ray, acc);
    else
        return nanovdb_RootData_float_getDimAndCache(cxt, ijk, ray, acc);
}
bool nanovdb_ReadAccessor_isActive(nanovdb_ReadAccessor acc, nanovdb_Coord ijk)
{
    return true;
}
vec3 nanovdb_Grid_worldToIndexF(nanovdb_GridData grid, const vec3 src)
{
    return nanovdb_Map_applyInverse(grid.mMap, src);
}
vec3 nanovdb_Grid_indexToWorldF(nanovdb_GridData grid, const vec3 src)
{
    return nanovdb_Map_apply(grid.mMap, src);
}
vec3 nanovdb_Grid_worldToIndexDirF(nanovdb_GridData grid, const vec3 src)
{
    return nanovdb_Map_applyInverseJacobi(grid.mMap, src);
}
vec3 nanovdb_Grid_indexToWorldDirF(nanovdb_GridData grid, const vec3 src)
{
    return nanovdb_Map_applyJacobi(grid.mMap, src);
}
nanovdb_Ray nanovdb_Ray_worldToIndexF(nanovdb_Ray ray, const nanovdb_GridData grid)
{
    nanovdb_Ray result;
    const vec3 eye = nanovdb_Grid_worldToIndexF(grid, ray.mEye);
    const vec3 dir = nanovdb_Grid_worldToIndexDirF(grid, ray.mDir);
    const float len = length(dir), invLength = float(1) / len;
    float t1 = ray.mT1;
    if (t1 < MaxFloat)
        t1 *= len;
    result.mEye = eye;
    result.mDir = ((dir) * (invLength));
    result.mT0 = len * ray.mT0;
    result.mT1 = t1;
    return result;
}
HDDA HDDA_create(nanovdb_Ray ray, int dim)
{
    vec3 dir = ray.mDir;
    vec3 inv = ((1.0f) / (dir));
    vec3 pos = ((ray.mEye) + (((ray.mT0) * (dir))));
    HDDA v;
    v.mDim = dim;
    v.mT0 = ray.mT0;
    v.mT1 = ray.mT1;
    v.mVoxel = ((ivec3(int(floor(pos.x)), int(floor(pos.y)), int(floor(pos.z)))) & ((~(dim - 1))));
    {
        if (abs(dir.x) < DeltaFloat)
        {
            v.mNext.x = MaxFloat;
        }
        else if (inv.x > 0)
        {
            v.mStep.x = dim;
            v.mNext.x = v.mT0 + (float(v.mVoxel.x) + float(dim) - pos.x) * inv.x;
            v.mDelta.x = dim * inv.x;
        }
        else
        {
            v.mStep.x = -dim;
            v.mNext.x = v.mT0 + (v.mVoxel.x - pos.x) * inv.x;
            v.mDelta.x = -dim * inv.x;
        }
    }
    {
        if (abs(dir.y) < DeltaFloat)
        {
            v.mNext.y = MaxFloat;
        }
        else if (inv.y > 0)
        {
            v.mStep.y = dim;
            v.mNext.y = v.mT0 + (v.mVoxel.y + dim - pos.y) * inv.y;
            v.mDelta.y = dim * inv.y;
        }
        else
        {
            v.mStep.y = -dim;
            v.mNext.y = v.mT0 + (v.mVoxel.y - pos.y) * inv.y;
            v.mDelta.y = -dim * inv.y;
        }
    }
    {
        if (abs(dir.z) < DeltaFloat)
        {
            v.mNext.z = MaxFloat;
        }
        else if (inv.z > 0)
        {
            v.mStep.z = dim;
            v.mNext.z = v.mT0 + (v.mVoxel.z + dim - pos.z) * inv.z;
            v.mDelta.z = dim * inv.z;
        }
        else
        {
            v.mStep.z = -dim;
            v.mNext.z = v.mT0 + (v.mVoxel.z - pos.z) * inv.z;
            v.mDelta.z = -dim * inv.z;
        }
    }
    return v;
}
bool HDDA_step(inout HDDA v)
{
    const int stepAxis = MinIndex(v.mNext);
    if (stepAxis == 0)
    {
        v.mT0 = v.mNext.x;
        v.mNext.x += v.mDelta.x;
        v.mVoxel.x += v.mStep.x;
    }
    else if (stepAxis == 1)
    {
        v.mT0 = v.mNext.y;
        v.mNext.y += v.mDelta.y;
        v.mVoxel.y += v.mStep.y;
    }
    else if (stepAxis == 2)
    {
        v.mT0 = v.mNext.z;
        v.mNext.z += v.mDelta.z;
        v.mVoxel.z += v.mStep.z;
    }
    return v.mT0 <= v.mT1;
}
bool HDDA_update(inout HDDA v, nanovdb_Ray ray, int dim)
{
    if (v.mDim == dim)
        return false;
    v.mDim = dim;
    vec3 dir = ray.mDir;
    vec3 inv = ((1.0f) / (dir));
    vec3 pos = ((ray.mEye) + (((ray.mT0) * (dir))));
    v.mVoxel = ((ivec3(int(floor(pos.x)), int(floor(pos.y)), int(floor(pos.z)))) & ((~(dim - 1))));
    if (abs(dir.x) >= DeltaFloat)
    {
        float tmp = dim * inv.x;
        v.mStep.x = dim;
        v.mDelta.x = tmp;
        v.mNext.x = v.mT0 + (v.mVoxel.x + dim - pos.x) * inv.x;
        if (inv.x <= 0)
        {
            v.mStep.x = -dim;
            v.mDelta.x = -tmp;
            v.mNext.x -= tmp;
        }
    }
    if (abs(dir.y) >= DeltaFloat)
    {
        float tmp = dim * inv.y;
        v.mStep.y = dim;
        v.mDelta.y = tmp;
        v.mNext.y = v.mT0 + (v.mVoxel.y + dim - pos.y) * inv.y;
        if (inv.y <= 0)
        {
            v.mStep.y = -dim;
            v.mDelta.y = -tmp;
            v.mNext.y -= tmp;
        }
    }
    if (abs(dir.z) >= DeltaFloat)
    {
        float tmp = dim * inv.z;
        v.mStep.z = dim;
        v.mDelta.z = tmp;
        v.mNext.z = v.mT0 + (v.mVoxel.z + dim - pos.z) * inv.z;
        if (inv.z <= 0)
        {
            v.mStep.z = -dim;
            v.mDelta.z = -tmp;
            v.mNext.z -= tmp;
        }
    }
    return true;
}
bool nanovdb_ZeroCrossing(int cxt, nanovdb_Ray ray, nanovdb_ReadAccessor acc, inout nanovdb_Coord ijk, inout float v)
{
    bool hit = nanovdb_Ray_clip(ray,
                                nanovdb_CoordToVec3f(kRootData.root.mBBox_min),
                                ((nanovdb_CoordToVec3f(kRootData.root.mBBox_max)) + (vec3(float(1), float(1), float(1)))));
    if (!hit || ray.mT1 > 1000000.f)
        return false;
    ijk = nanovdb_Vec3fToCoord(nanovdb_Ray_start(ray));
    float v0 = nanovdb_ReadAccessor_getValue(cxt, acc, ijk);
    int n = int(ray.mT1 - ray.mT0);
    if (n <= 1)
        return false;
    HDDA hdda = HDDA_create(ray, 1);
    while (HDDA_step(hdda) && (--n > 0))
    {
        ijk = nanovdb_Vec3fToCoord(((ray.mEye) + ((((hdda.mT0 + 1.0f)) * (ray.mDir)))));
        if (hdda.mDim < 1 || !nanovdb_ReadAccessor_isActive(acc, ijk))
            continue;
        while (HDDA_step(hdda) && nanovdb_ReadAccessor_isActive(acc, nanovdb_Vec3iToCoord(hdda.mVoxel)))
        {
            ijk = nanovdb_Vec3iToCoord(hdda.mVoxel);
            v = nanovdb_ReadAccessor_getValue(cxt, acc, ijk);
            if (v * v0 < 0)
                return true;
        }
    }
    return false;
}
uniform ArgUniforms
{
    int width;
    int height;
    int numAccumulations;
    int useBackground;
    int useGround;
    int useShadows;
    int useGroundReflections;
    int useLighting;
    float useOcclusion;
    float volumeDensityScale;
    float volumeAlbedo;
    int useTonemapping;
    float tonemapWhitePoint;
    int samplesPerPixel;
    float groundHeight;
    float groundFalloff;
    float cameraPx, cameraPy, cameraPz;
    float cameraUx, cameraUy, cameraUz;
    float cameraVx, cameraVy, cameraVz;
    float cameraWx, cameraWy, cameraWz;
    float cameraAspect;
    float cameraFovY;
}
kArgs;
float reinhardFn(float x)
{
    return x / (1.0f + x);
}
float invReinhardFn(float x)
{
    return x / (1.0f - x);
}
void tonemapReinhard(inout vec3 outColor, vec3 inColor, float W)
{
    const float invW = 1.0f / reinhardFn(W);
    outColor = vec3(float(reinhardFn(inColor.x) * invW), float(reinhardFn(inColor.y) * invW), float(reinhardFn(inColor.z) * invW));
}
void invTonemapReinhard(inout vec3 outColor, vec3 inColor, float W)
{
    const float w = reinhardFn(W);
    outColor = vec3(float(invReinhardFn(inColor.x * w)), float(invReinhardFn(inColor.y * w)), float(invReinhardFn(inColor.z * w)));
}
uint hash1(uint x)
{
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}
uint hash2(uint x, uint y)
{
    return hash1(x ^ hash1(y));
}
float randomf(uint s)
{
    return float(hash1(s)) / float(0xffffffffu);
}
vec3 makeVec3(float x)
{
    return vec3(float(x), float(x), float(x));
}
float evalSkyMaterial(const vec3 dir)
{
    return 0.75f + 0.25f * dir.y;
}
float mysmoothstep(float t, float edge0, float edge1)
{
    t = (t - edge0) / (edge1 - edge0);
    t = max(0.f, min(1.f, t));
    return t * t * (3.0f - 2.0f * t);
}
float evalGroundMaterial(float wGroundT, float wFalloffDistance, const vec3 pos, float rayDirY, inout float outMix)
{
    const float s = min(wGroundT / wFalloffDistance, 1.f);
    outMix = max(0.f, (1.0f - s) * -rayDirY);
    const float checkerScale = 1.0f / float(1 << (3 + 4));
    float iu = floor(pos.x * checkerScale);
    float iv = floor(pos.z * checkerScale);
    float floorIntensity = 0.25f + abs(mod(iu + iv, 2.f)) * 0.5f;
    float t = max(mod(abs(pos.x), 1.0f), mod(abs(pos.z), 1.0f));
    const float lineWidth = s;
    float grid = mysmoothstep(t, 0.97f - lineWidth, 1.0f - lineWidth);
    grid *= max(0.f, (1.0f - (wGroundT / 100.f))) * -rayDirY;
    return floorIntensity + grid;
}
vec3 lambertNoTangent(vec3 normal, float u, float v)
{
    float theta = 6.283185f * u;
    v = 2.0f * v - 1.0f;
    float d = sqrt(1.0f - v * v);
    vec3 spherePoint = vec3(float(d * cos(theta)), float(d * sin(theta)), float(v));
    return normalize(((normal) + (spherePoint)));
}
vec3 getRayDirFromPixelCoord(uint ix, uint iy, int width, int height, int numAccumulations, uint pixelSeed, const vec3 cameraU, const vec3 cameraV, const vec3 cameraW, float fovY, float aspect)
{
    float u = float(ix) + 0.5f;
    float v = float(iy) + 0.5f;
    float randVar1 = randomf(pixelSeed + 0);
    float randVar2 = randomf(pixelSeed + 1);
    if (numAccumulations > 0)
    {
        u += randVar1 - 0.5f;
        v += randVar2 - 0.5f;
    }
    u /= float(width);
    v /= float(height);
    float halfHeight = tan(fovY * 3.14159265358979323846f / 360.f);
    float halfWidth = aspect * halfHeight;
    vec3 W = ((((((halfWidth) * (cameraU))) + (((halfHeight) * (cameraV))))) + (cameraW));
    vec3 U = ((2.f * halfWidth) * (cameraU));
    vec3 V = ((2.f * halfHeight) * (cameraV));
    vec3 rd = ((((((u) * (U))) + (((v) * (V))))) - (W));
    return normalize(rd);
}
void compositeFn(int image, int width, ivec2 tid, vec3 color, int numAccumulations, int useTonemapping, float tonemapWhitePoint)
{
    if (numAccumulations > 1)
    {
        vec4 prevOutput = imageLoad(kOutImage, tid);
        ;
        vec3 oldLinearPixel;
        vec3 prevColor = vec3(float(prevOutput.x), float(prevOutput.y), float(prevOutput.z));
        if (useTonemapping != 0)
        {
            invTonemapReinhard(oldLinearPixel, prevColor, tonemapWhitePoint);
        }
        else
        {
            oldLinearPixel = prevColor;
        }
        color = ((oldLinearPixel) + ((((1.0f / float(numAccumulations))) * (((color) - (oldLinearPixel))))));
    }
    if (useTonemapping != 0)
    {
        tonemapReinhard(color, color, tonemapWhitePoint);
    }
    imageStore(kOutImage, tid, vec4(float(color.x), float(color.y), float(color.z), float(1.0f)));
}
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
    int cxt;
    int outImage;
    ivec2 threadId = ivec2(gl_GlobalInvocationID.xy);
    uint ix = threadId.x;
    uint iy = threadId.y;
    if (ix >= uint(kArgs.width) || iy >= uint(kArgs.height))
        return;
    uint pixelSeed = kArgs.numAccumulations + hash2(ix, iy);
    vec3 cameraP = vec3(float(kArgs.cameraPx), float(kArgs.cameraPy), float(kArgs.cameraPz));
    vec3 cameraU = vec3(float(kArgs.cameraUx), float(kArgs.cameraUy), float(kArgs.cameraUz));
    vec3 cameraV = vec3(float(kArgs.cameraVx), float(kArgs.cameraVy), float(kArgs.cameraVz));
    vec3 cameraW = vec3(float(kArgs.cameraWx), float(kArgs.cameraWy), float(kArgs.cameraWz));
    const vec3 wLightDir = vec3(float(0.0f), float(1.0f), float(0.0f));
    const vec3 iLightDir = normalize(nanovdb_Grid_worldToIndexDirF(kGridData.grid, wLightDir));
    nanovdb_ReadAccessor acc = nanovdb_ReadAccessor_create();
    vec3 color = vec3(float(0), float(0), float(0));
    int sampleIndex = 0;
    {
        uint pixelSeed = hash1(sampleIndex + kArgs.numAccumulations * kArgs.samplesPerPixel ^ hash2(ix, iy));
        float randVar1 = randomf(pixelSeed + 0);
        float randVar2 = randomf(pixelSeed + 1);
        vec3 wRayDir = getRayDirFromPixelCoord(ix, iy, kArgs.width, kArgs.height, kArgs.numAccumulations, pixelSeed, cameraU, cameraV, cameraW, kArgs.cameraFovY, kArgs.cameraAspect);
        vec3 wRayEye = cameraP;
        nanovdb_Ray wRay;
        wRay.mEye = wRayEye;
        wRay.mDir = wRayDir;
        wRay.mT0 = 0;
        wRay.mT1 = MaxFloat;
        nanovdb_Ray iRay = nanovdb_Ray_worldToIndexF(wRay, kGridData.grid);
        vec3 iRayDir = iRay.mDir;
        nanovdb_Coord ijk;
        float v0 = 0.0f;
        if (nanovdb_ZeroCrossing(cxt, iRay, acc, ijk, v0))
        {
            vec3 iPrimaryPos = nanovdb_CoordToVec3f(ijk);
            vec3 iNormal = makeVec3(-v0);
            ijk.mVec[0] += 1;
            iNormal.x += nanovdb_ReadAccessor_getValue(cxt, acc, ijk);
            ijk.mVec[0] -= 1;
            ijk.mVec[1] += 1;
            iNormal.y += nanovdb_ReadAccessor_getValue(cxt, acc, ijk);
            ijk.mVec[1] -= 1;
            ijk.mVec[2] += 1;
            iNormal.z += nanovdb_ReadAccessor_getValue(cxt, acc, ijk);
            iNormal = normalize(iNormal);
            vec3 intensity = makeVec3(1);
            float occlusion = 0.0f;
            if (kArgs.useOcclusion > 0)
            {
                nanovdb_Ray iOccRay;
                iOccRay.mEye = ((iPrimaryPos) + (((2.0f) * (iNormal))));
                iOccRay.mDir = lambertNoTangent(iNormal, randVar1, randVar2);
                iOccRay.mT0 = DeltaFloat;
                iOccRay.mT1 = MaxFloat;
                if (nanovdb_ZeroCrossing(cxt, iOccRay, acc, ijk, v0))
                    occlusion = kArgs.useOcclusion;
                intensity = makeVec3(1.0f - occlusion);
            }
            if (kArgs.useLighting > 0)
            {
                vec3 diffuseKey = makeVec3(1);
                vec3 diffuseFill = makeVec3(1);
                float ambient = 1.0f;
                float specularKey = 0.f;
                float shadowFactor = 0.0f;
                vec3 H = normalize(((iLightDir) - (iRayDir)));
                specularKey = pow(max(0.0f, dot(iNormal, H)), 10.f);
                const float diffuseWrap = 0.25f;
                diffuseKey = ((max(0.0f, (dot(iNormal, iLightDir) + diffuseWrap) / (1.0f + diffuseWrap))) * (diffuseKey));
                diffuseFill = ((max(0.0f, -dot(iNormal, iRayDir))) * (diffuseFill));
                if (kArgs.useShadows > 0)
                {
                    nanovdb_Ray iShadowRay;
                    iShadowRay.mEye = ((iPrimaryPos) + (((2.0f) * (iNormal))));
                    iShadowRay.mDir = iLightDir;
                    iShadowRay.mT0 = DeltaFloat;
                    iShadowRay.mT1 = MaxFloat;
                    if (nanovdb_ZeroCrossing(cxt, iShadowRay, acc, ijk, v0))
                        shadowFactor = kArgs.useShadows;
                }
                intensity = (((1.0f - shadowFactor)) * (((makeVec3(specularKey * 0.2f)) + (((0.8f) * (diffuseKey))))));
                intensity = ((intensity) + ((((1.0f - occlusion)) * (((((0.2f) * (diffuseFill))) + (makeVec3(ambient * 0.1f)))))));
                intensity = ((kArgs.useLighting) * (intensity));
                intensity = ((intensity) + (makeVec3((1.0f - kArgs.useLighting) * (1.0f - occlusion))));
            }
            color.x += intensity.x;
            color.y += intensity.y;
            color.z += intensity.z;
        }
        else
        {
            float bgIntensity = 0.0f;
            if (kArgs.useBackground > 0)
            {
                float groundIntensity = 0.0f;
                float groundMix = 0.0f;
                if (kArgs.useGround > 0)
                {
                    float wGroundT = (kArgs.groundHeight - wRayEye.y) / wRayDir.y;
                    if (wRayDir.y != 0 && wGroundT > 0.f)
                    {
                        vec3 wGroundPos = ((wRayEye) + (((wGroundT) * (wRayDir))));
                        vec3 iGroundPos = nanovdb_Grid_worldToIndexF(kGridData.grid, wGroundPos);
                        vec3 iGroundNormal = vec3(float(0), float(1), float(0));
                        groundIntensity = evalGroundMaterial(wGroundT, kArgs.groundFalloff, wGroundPos, wRayDir.y, groundMix);
                        if (kArgs.useOcclusion > 0)
                        {
                            nanovdb_Ray iOccRay;
                            iOccRay.mEye = ((iGroundPos) + (((2.0f) * (iGroundNormal))));
                            iOccRay.mDir = lambertNoTangent(iGroundNormal, randVar1, randVar2);
                            iOccRay.mT0 = DeltaFloat;
                            iOccRay.mT1 = MaxFloat;
                            if (nanovdb_ZeroCrossing(cxt, iOccRay, acc, ijk, v0))
                                groundIntensity = groundIntensity * (1.0f - kArgs.useOcclusion);
                        }
                        if (kArgs.useShadows > 0)
                        {
                            nanovdb_Ray iShadowRay;
                            iShadowRay.mEye = iGroundPos;
                            iShadowRay.mDir = iLightDir;
                            iShadowRay.mT0 = DeltaFloat;
                            main.cpp : 392 : error : unterminated #if 392 | #define CNANOVDB_CONTEXT const TreeContext *

                                                                                iShadowRay.mT1 = MaxFloat;
                            if (nanovdb_ZeroCrossing(cxt, iShadowRay, acc, ijk, v0))
                                groundIntensity = 0;
                        }
                        if (kArgs.useGroundReflections > 0)
                        {
                            nanovdb_Ray iReflRay;
                            iReflRay.mEye = iGroundPos;
                            iReflRay.mDir = ((iRayDir) - (vec3(float(0.f), float(2.0f * iRayDir.y), float(0.f))));
                            iReflRay.mT0 = DeltaFloat;
                            iReflRay.mT1 = MaxFloat;
                            if (nanovdb_ZeroCrossing(cxt, iReflRay, acc, ijk, v0))
                                groundIntensity = 0;
                        }
                    }
                }
                float skyIntensity = evalSkyMaterial(wRayDir);
                bgIntensity = (1.f - groundMix) * skyIntensity + groundMix * groundIntensity;
            }
            color.x += bgIntensity;
            color.y += bgIntensity;
            color.z += bgIntensity;
        }
    }
    color.x /= kArgs.samplesPerPixel;
    color.y /= kArgs.samplesPerPixel;
    color.z /= kArgs.samplesPerPixel;
    compositeFn(outImage, kArgs.width, threadId, color, kArgs.numAccumulations, kArgs.useTonemapping, kArgs.tonemapWhitePoint);
}
