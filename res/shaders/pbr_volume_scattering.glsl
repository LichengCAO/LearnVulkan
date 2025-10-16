#ifndef VOLUME_SCATTERING_GLSL
#define VOLUME_SCATTERING_GLSL
#include "pbr_common.glsl"

// g: anisotropy parameter, -1 <= g <= 1
// g = 0 isotropic
// g's physical meaning: https://pbr-book.org/4ed/Volume_Scattering/Phase_Functions#eq:scattering-anisotropy
float _HenyeyGreensteinPhaseFunction(in float cosTheta, in float g)
{
    float g2 = g * g;
    return (1.0f - g2) / pow(1.0f + g2 - 2.0f * g * cosTheta, 1.5f) * INV_PI * 0.25f;
}

// for volume scattering
// wo, wi poiting outwards, in world space
void _SampleHenyeyGreenstein(
    in vec3 wo, 
    in float g,
    in vec2 xi, 
    out vec3 wi, 
    out float pdf)
{
    float cosTheta = 0.0f;
    float phi = 2.0f * PI * xi.y;
    if (abs(g) < 1e-3f)
    {
        // isotropic
        cosTheta = 1.0f - 2.0f * xi.x;
        pdf = _HenyeyGreensteinPhaseFunction(cosTheta, 0.0f);
    }
    float sinTheta = sqrt(max(0.0f, 1.0f - cosTheta * cosTheta));
    vec3 wiTangent = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
    wi = TangentToWorld(wo, wiTangent);
}

#endif // VOLUME_SCATTERING_GLSL