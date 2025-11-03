#ifndef VOLUME_SCATTERING_GLSL
#define VOLUME_SCATTERING_GLSL
#include "pbr_common.glsl"

// sigma_a: absorption coefficient
// sigma_s: scattering coefficient
// sigma_t = sigma_a + sigma_s: extinction coefficient
// sigma_majorant = sigma_t for homogeneous medium

// pdf: a * exp(-a * xi)
float SampleExponential(in float xi, in float a)
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
    out float phase_function)
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
    phase_function = _HenyeyGreensteinPhaseFunction(cosTheta, g);
    pdf = phase_function; // true for henyey greenstein
}

// evaluate second term of 14.3 in PBRTv4
void Ln(
    in vec3 rand,
    in float g,
    in float sigma_a,
    in float sigma_s,
    in float sigma_n,
    inout vec3 rayDirection,
    inout vec3 throughput,
    out uint state) // 0: absorb 1: scatter 2: null
{
    float sigma_majorant = sigma_a + sigma_s + sigma_n;

    // first term in 14.5
    if (sigma_a / sigma_majorant > rand.x)
    {
        // ends here
        // should apply Le(p')
        // since we have no emission in homogeneous medium, we ends like t1 > t
        vec3 Le = vec3(0.0f); // if there is emission, we need to tell that do not proceed anymore
        throughput *= Le;
        state = 0;
    }
    // second term in 14.5
    else if ((sigma_a + sigma_s) / sigma_majorant > rand.x)
    {
        vec3 nextRayDirection;
        float phase_function = 1.0f;
        float pdf = 1.0f;
        _SampleHenyeyGreenstein(-rayDirection, g, vec2(rand.y, rand.z), nextRayDirection, pdf, phase_function);
        rayDirection = nextRayDirection;
        throughput *= (phase_function / pdf); // weighted by the ratio of the phase function and the probability of sampling the direction, don't know why
        state = 1;
    }
    // third term in 14.5
    else
    {
        state = 3;
    }
}

void SampleHomogeneousVolume(
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

    t1 = SampleExponential(u, sigma_majorant); // t'
    if (t1 > t)
    {
        // evaluate first term of 14.3 in PBRTv4
        // evaluate with BXDF
        rayOrigin = rayOrigin + t * rayDirection;
        state = 0;
    }
    else
    {
        // don't know how do we arrive 14.7's second formula in pbrtv4 here, why 1-q is eliminated?
        // maybe because: 
        // we have samples in 0-t instead of 0-inifinity 
        // and with monte carlo integration our pdf becomes pdf_origin / (1 - q) 
        // the result becomes integral from 0 to t instead of 0 to infinity
        uint LnState;
        rayOrigin = rayOrigin + t1 * rayDirection;
        // we need update sigma_a, sigma_s, sigma_n here for heterogeneous medium

        Ln(rand2, g, sigma_a, sigma_s, sigma_n, rayDirection, throughput, LnState);
        if (LnState == 0) state = 1;
        else state = 2;
    }
}

#endif // VOLUME_SCATTERING_GLSL