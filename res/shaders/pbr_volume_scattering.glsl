#ifndef VOLUME_SCATTERING_GLSL
#define VOLUME_SCATTERING_GLSL
#include "pbr_common.glsl"

// sigma_a: absorption coefficient
// sigma_s: scattering coefficient
// sigma_t = sigma_a + sigma_s: extinction coefficient
// sigma_majorant = sigma_t for homogeneous medium

// pdf: a * exp(-a * xi)
float _SampleExponential(in float xi, in float a)
{
    return -log(1.0f - xi) / a;
}

// g: anisotropy parameter, -1 < g < 1
// g = 0 isotropic
// g's physical meaning: https://pbr-book.org/4ed/Volume_Scattering/Phase_Functions#eq:scattering-anisotropy
float _HenyeyGreensteinPhaseFunction(in float cosTheta, in float g)
{
    float g2 = g * g;

    return (1.0f - g2) / pow(1.0f + g2 - 2.0f * g * cosTheta, 1.5f) * INV_PI * 0.25f;
}

// for volume scattering
// wo, wi poiting outwards, in world space
// apply bxdf to throughput
void _SampleHenyeyGreenstein(
    in vec3 wo, 
    in float g,
    in vec2 xi, 
    out vec3 wi,
    out float pdf,
    inout vec3 throughput)
{
    float cosTheta = 0.0f;
    float phi = 2.0f * PI * xi.y;
    if (abs(g) < 1e-3f)
    {
        // isotropic
        cosTheta = 1.0f - 2.0f * xi.x;
    }
    else
    {
        cosTheta = -1.0 / (2 * g) * (1.0f + Sqr(g) - Sqr((1.0 - Sqr(g)) / (1.0f + g - 2 * g * xi.x)));
    }
    float sinTheta = sqrt(max(0.0f, 1.0f - cosTheta * cosTheta));
    vec3 wiTangent = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
    wi = TangentToWorld(wo, wiTangent);
    float p = _HenyeyGreensteinPhaseFunction(cosTheta, g);
    pdf = p;
    throughput *= p; // still don't know why is this
}

void SampleVolume(
    in vec3 rand,
    in vec3 rand2,
    in float t,             // distance from ray origin to surface hit
    in float g,             // HenyeyGreenstein
    in float sigma_a,
    in float sigma_s,
    in float sigma_n,
    inout vec3 throughput, // total light
    inout vec3 rayOrigin,
    inout vec3 rayDirection,
    out uint state)        // 0: BXDF 1: trace ends 2: call sample volume again
{
    float t1; // t'
    const float sigma_majorant = sigma_a + sigma_s + sigma_n;
    const float u = rand.x;
    const float u2 = rand.y;
    const float u3 = rand.z;
    const float u4 = rand2.x;
    const float u5 = rand2.y;

    t1 = _SampleExponential(u, sigma_majorant); // t' 
    if (t1 > t)
    {
        // evaluate first term of 14.3 in PBRTv4
        // evaluate with BXDF
        rayOrigin = rayOrigin + t * rayDirection;
        state = 0;
    }
    else
    {
        t1 = _SampleExponential(u2, sigma_majorant); // resample t' for next position in medium
        if (t1 > t)
        {
            // no absorption or scattering before surface hit, ends here
            throughput = vec3(0.0f);
            state = 1;
        }
        // evaluate second term of 14.3 in PBRTv4
        if (sigma_a / sigma_majorant > u3)
        {
            // first term in 14.5, ends here
            // should apply Le(p')
            // since we have no emission in homogeneous medium, we ends like t1 > t
            throughput = vec3(0.0f); // if there is emission, we need to tell that do not proceed anymore
            state = 1;
        }
        else if ((sigma_a + sigma_s) / sigma_majorant > u3)
        {
            // second term in 14.5
            vec3 nextRayDirection;
            float pdf = 1.0f;
            _SampleHenyeyGreenstein(-rayDirection, g, vec2(u4, u5), nextRayDirection, pdf, throughput);
            rayOrigin = rayOrigin + t1 * rayDirection;
            rayDirection = nextRayDirection;
            throughput /= pdf;
            // apply SampleHomogenousMedium again
            state = 2;
        }
        else
        {
            // third term in 14.5
            rayOrigin = rayOrigin + t1 * rayDirection;
            // apply SampleHomogenousMedium again
            state = 2;
        }
    }
}

#endif // VOLUME_SCATTERING_GLSL