struct PBRPayload
{
    vec3 hitValue;
    vec3 rayOrigin;
    vec3 rayDirection;
    bool traceEnd;
};

struct PBRMaterial
{
    vec4 baseColor;
};

#define PI 3.14159265359
#define INV_PI 0.31830988618

uint _ReverseBits32(uint n) 
{
    n = (n << 16) | (n >> 16);
    n = ((n & 0x00ff00ff) << 8) | ((n & 0xff00ff00) >> 8);
    n = ((n & 0x0f0f0f0f) << 4) | ((n & 0xf0f0f0f0) >> 4);
    n = ((n & 0x33333333) << 2) | ((n & 0xcccccccc) >> 2);
    n = ((n & 0x55555555) << 1) | ((n & 0xaaaaaaaa) >> 1);
    return n;
}

// 2 base radical inverse
float _RadicalInverse(uint _n)
{
    return float(_ReverseBits32(_n)) * 2.3283064365386963e-10;// (2^-32);
}

vec2 Hammersley(uint a, uint N)
{
    return vec2(float(a) / float(N), _RadicalInverse(a));
}

// Hash Functions for GPU Rendering, Jarzynski et al.
// http://www.jcgt.org/published/0009/03/02/
// code from https://www.gsn-lib.org/apps/raytracing/index.php?name=example_pathtracing
vec3 random_pcg3d(uvec3 v) {
  v = v * 1664525u + 1013904223u;
  v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
  v ^= v >> 16u;
  v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
  return vec3(v) * (1.0/float(0xffffffffu));
}
