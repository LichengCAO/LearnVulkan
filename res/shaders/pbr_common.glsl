#ifndef RT_PBR_COMMON_GLSL
#define RT_PBR_COMMON_GLSL
struct PBRPayload
{
    vec3 hitValue;
    vec3 rayOrigin;
    vec3 rayDirection;
    bool traceEnd;
    bool volumeScatter;
    uint randomSeed;
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

float Sqr(in float x)
{
    return x * x;
}

// Hash Functions for GPU Rendering, Jarzynski et al.
// http://www.jcgt.org/published/0009/03/02/
// code from https://www.gsn-lib.org/apps/raytracing/index.php?name=example_pathtracing
vec3 pcg3d(uvec3 v) 
{
    v = v * 1664525u + 1013904223u;
    v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
    v ^= v >> 16u;
    v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
    return vec3(v) * (1.0/float(0xffffffffu));
}

//https://observablehq.com/@riccardoscalco/pcg-random-number-generators-in-glsl
vec3 pcg3d_2(uvec3 v)
{
    v = v * uint(747796405) + uint(2891336453);
    v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
    v ^= v >> 16u;
    v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
    return vec3(v) * (1.0/float(0xffffffffu));
}

vec3 CosineWeightedSample(const vec2 xi)
{
    // float phi = 2.0f * PI * xi.x;
    ////float theta = asin(xi.y); // i think it's right if we only need to consider theta, but we need to consider both phi and theta
    // float theta = asin(sqrt(xi.y));
    // return vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));

    // see: https://pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations#CosineSampleHemisphere
    float phi = 2.0f * PI * xi.x;
    float sinTheta = sqrt(xi.y);
    return vec3(sinTheta * cos(phi), sinTheta * sin(phi), sqrt(1.0 - xi.y));
}
vec3 WorldToTangent(const vec3 _normal, const vec3 _sample)
{
    vec3 tangent = abs(_normal.z) < 0.999f ? normalize(cross(vec3(0.0f, 0.0f, 1.0f), _normal)) : vec3(1.0f, 0.0f, 0.0f);
    vec3 bitangent = cross(_normal, tangent);
    return vec3(dot(_sample, tangent), dot(_sample, bitangent), dot(_sample, _normal));
}
vec3 TangentToWorld(const vec3 _normal, const vec3 _sample)
{
    vec3 tangent = abs(_normal.z) < 0.999f ? normalize(cross(vec3(0.0f, 0.0f, 1.0f), _normal)) : vec3(1.0f, 0.0f, 0.0f);
    vec3 bitangent = cross(_normal, tangent);
    return _sample.x * tangent + _sample.y * bitangent + _sample.z * _normal;
}
// https://blog.selfshadow.com/sandbox/ltc.html
vec3 TangentToWorld(const vec3 _normal, const vec3 _view, const vec3 _sample)
{
    vec3 tangent = normalize(_view - _normal * dot(_view, _normal));
    vec3 bitangent = cross(_normal, tangent);
    return _sample.x * tangent + _sample.y * bitangent + _sample.z * _normal;
}

vec2 UniformDiskSample(const vec2 xi)
{
    float r = sqrt(xi.x);
    float theta = 2.0f * PI * xi.y;
    return vec2(r * cos(theta), r * sin(theta));
}

// pbr
float Cos2Theta(in vec3 w)
{
    return w.z * w.z;
}
float CosTheta(in vec3 w)
{
    return w.z;
}
float Sin2Theta(in vec3 w)
{
    return max(0.0f, 1.0f - Cos2Theta(w));
}
float SinTheta(in vec3 w)
{
    return sqrt(Sin2Theta(w));
}
float Tan2Theta(in vec3 w)
{
    return Sin2Theta(w) / Cos2Theta(w);
}
float TanTheta(in vec3 w)
{
    return sqrt(Tan2Theta(w));
}
float CosPhi(in vec3 w)
{
    float sinTheta = SinTheta(w);
    if (sinTheta == 0.0f) return 1.0f;
    return clamp(w.x / sinTheta, -1.0f, 1.0f);
}
float SinPhi(in vec3 w)
{
    float sinTheta = SinTheta(w);
    if (sinTheta == 0.0f) return 0.0f;
    return clamp(w.y / sinTheta, -1.0f, 1.0f);
}
#endif // RT_PBR_COMMON_GLSL