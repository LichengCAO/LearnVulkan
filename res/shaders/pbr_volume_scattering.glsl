#ifndef VOLUME_SCATTERING_GLSL
#define VOLUME_SCATTERING_GLSL
#include "pbr_common.glsl"

// sigma_a: absorption coefficient
// sigma_s: scattering coefficient
// sigma_t = sigma_a + sigma_s: extinction coefficient
// sigma_majorant = sigma_t for homogeneous medium

struct Medium
{
    vec3 sigma_a; // absorption coefficient
    vec3 sigma_s; // scattering coefficient
    float g;      // anisotropy parameter for Henyey-Greenstein phase function
}

struct SegmentMajorant
{
    float tMin;
    float tMax;
    float majorant; // majorant extinction coefficient
}

// pdf: a * exp(-a * xi)
float _SampleExponential(in float xi, in float a)
{
    return -log(1.0f - xi) / a;
}

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

void _SamplePointInMedium(
    in Medium medium,
    in vec3 point,
    out vec3 sigma_a,
    out vec3 sigma_s,
    out vec3 Le,
    out float g)
{
    sigma_a = medium.sigma_a;
    sigma_s = medium.sigma_s;
    Le = vec3(0.0f);
    g = medium.g;
}

void SampleHomogenousMedium(
    in float xi,
    in float t, // distance from ray origin to surface hit
    in float g,
    in float sigma_a,
    in float sigma_s,
    in float sigma_n,
    inout vec3 throughput, // total light
    inout vec3 rayOrigin,
    inout vec3 rayDirection,
    out bool endMedium)
{
    float t1; // t'
    float sigma_majorant = sigma_a + sigma_s + sigma_n;
    float u = xi;
    float u2 = xi;
    float u3 = xi;
    float u4 = xi;
    float u5 = xi;

    t1 = _SampleExponential(u, sigma_majorant); // t' 
    if (t1 > t)
    {
        // evaluate first term of 14.3 in PBRTv4
        // evaluate with BXDF
        rayOrigin = rayOrigin + t * rayDirection;
        endMedium = true; // do BXDF at surface
        return;
    }
    else
    {
        t1 = _SampleExponential(u2, sigma_majorant); // resample t' for next position in medium
        if (t1 > t)
        {
            // no absorption or scattering before surface hit, ends here
            endMedium = true;
            throughput = vec3(0.0f);
            return;
        }
        // evaluate second term of 14.3 in PBRTv4
        if (sigma_a / sigma_majorant < u3)
        {
            // first term in 14.5, ends here
            // should apply Le(p')
            // since we have no emission in homogeneous medium, we ends like t1 > t
            endMedium = true;
            throughput = vec3(0.0f); // if there is emission, we need to tell that do not proceed anymore
            return;
        }
        else if ((sigma_a + sigma_s) / sigma_majorant < u3)
        {
            // second term in 14.5
            vec3 nextRayDirection;
            _SampleHenyeyGreenstein(-rayDirection, g, vec2(u4, u5), nextRayDirection, pdf);
            rayOrigin = rayOrigin + t1 * rayDirection;
            rayDirection = nextRayDirection;
            throughput /= pdf; // TODO?: weighted by the ratio of the phase function and the probability of sampling the direction
            // apply SampleHomogenousMedium again
            endMedium = false;
        }
        else
        {
            // third term in 14.5
            rayOrigin = rayOrigin + t1 * rayDirection;
            // apply SampleHomogenousMedium again
            endMedium = false;
        }
    }
}

#endif // VOLUME_SCATTERING_GLSL