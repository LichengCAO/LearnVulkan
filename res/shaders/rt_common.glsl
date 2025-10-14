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

const float shadowValue = 0.05f; // Default shadow value
#endif // RT_COMMON_GLSL