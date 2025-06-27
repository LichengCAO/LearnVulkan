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
    vec3 position; // Position of the vertex in object space
    vec3 normal;   // Normal of the vertex in object space
};