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