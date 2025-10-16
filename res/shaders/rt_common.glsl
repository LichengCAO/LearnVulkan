#ifndef RT_COMMON_GLSL
#define RT_COMMON_GLSL
struct HitPayload
{
    vec3 hitValue;
    vec3 rayOrigin;
    vec3 rayDirection;
};

// Information of a obj model when referenced in a shader
struct InstanceAddressData
{
  uint64_t vertexAddress;         // Address of the Vertex buffer
  uint64_t indexAddress;          // Address of the index buffer
};

struct Vertex
{
    vec4 position; // Position of the vertex in object space
    vec4 normal;   // Normal of the vertex in object space
};

vec3 Reflect(const vec3 wi, const vec3 N)
{
    return 2.0 * dot(N, wi) * N - wi;
}

// Snell's law refraction
// IORi: IOR of material where Normal is pointing to
// IORt: IOR of material where Normal is pointing from
vec3 Refract(const vec3 wi, vec3 N, const float IORi, const float IORt)
{
    float eta = IORi / IORt;
    float cosI = dot(wi, N);
    if (cosI < 0.0f)
    {
      eta = 1.0f / eta;
      cosI = -cosI;
      N = -N;
    }
    float sinT2 = eta * eta * max(0.0f, 1.0 - cosI * cosI);
    if (sinT2 > 1.0)
      return vec3(0.0f); // Total internal reflection
    float cosT = sqrt(1.0 - sinT2);
    return (-eta * wi + (eta * cosI - cosT) * N);
}

const float shadowValue = 0.05f; // Default shadow value
#endif // RT_COMMON_GLSL