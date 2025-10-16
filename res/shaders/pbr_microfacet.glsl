#ifndef MICROFACET_GLSL
#define MICROFACET_GLSL
#include "pbr_common.glsl"

// https://balyberdin.com/tools/ior-list/#metal-material-ior

// roughness to alpha
float _RoughnessToAlpha(in float roughness)
{
    return roughness * roughness;
}

// lambda function
float _Lambda(in vec3 wo, in float alpha)
{
    float tan2Theta = Tan2Theta(wo);
    // TODO: case nan
    float alpha2 = alpha * alpha;
    return (sqrt(1.0f + alpha2 * tan2Theta) - 1.0f) / 2.0f;
}

// normal distribution function
float D(in vec3 wm, in float alpha)
{
    float tan2Theta = Tan2Theta(wm);
    float cos4Theta = Cos2Theta(wm) * Cos2Theta(wm);
    float alpha2 = alpha * alpha;
    return 1.0f / (PI * cos4Theta * alpha2 * (1.0f + tan2Theta / alpha2) * (1.0f + tan2Theta / alpha2));
}

// mask function
float G1(in vec3 wo, in float alpha)
{
    return 1.0f / (1.0f + _Lambda(wo, alpha)); 
}

float G(in vec3 wi, in vec3 wo, in float alpha)
{
    // use G1 * G1 overestimates the shadowing-masking function
    return 1.0f / (1.0f + _Lambda(wi, alpha) + _Lambda(wo, alpha));
}

// visible normal distribution function
// wo: to camera
// wm: microfacet normal
float Dw(in vec3 wo, in vec3 wm, in float alpha)
{
    return G1(wo, alpha) / CosTheta(wo) * abs(dot(wo, wm)) * D(wm, alpha);
}

// Sample microfacet normal wm given wo
void SampleMicrofacetNormal(in vec3 wo, in float roughness, in vec2 xi, out vec3 wm, out float pdf)
{
    float alpha = _RoughnessToAlpha(roughness);
    // Sample wh
    vec3 wh = normalize(vec3(wo.x * alpha, wo.y * alpha, wo.z));
    vec3 T1 = wh.z < 0.999f ? normalize(cross(wh, vec3(0.0f, 0.0f, 1.0f))) : vec3(1.0f, 0.0f, 0.0f);
    vec3 T2 = cross(wh, T1);

    // sample wi
    vec2 sampleDisk = UniformDiskSample(xi);

    // hemisphere proj
    float h = sqrt(1.0 - sampleDisk.x * sampleDisk.x);
    float cosTheta = CosTheta(wo);
    float scale = 0.5 * (1.0 + cosTheta);
    float offset = 0.5 * h * (1.0 - cosTheta);
    
    // remap y from [-h, h] to [-cosTheta*h, h]
    sampleDisk.y = offset + sampleDisk.y * scale;

    wm = sampleDisk.x * T1 + sampleDisk.y * T2 + sqrt(max(0.0f, 1.0f - sampleDisk.x * sampleDisk.x - sampleDisk.y * sampleDisk.y)) * wh;

    // transform back
    wm = normalize(vec3(wm.x * alpha, wm.y * alpha, max(0.000001f, wm.z)));

    float pdf_wm = Dw(wo, wm, alpha);
    pdf = pdf_wm / (4.0f * abs(dot(wo, wm)));
}

vec3 MicrofacetBRDF(in vec3 wo, in vec3 wm, in float roughness)
{
    if (dot(wm, wm) < 0.0001f) return vec3(0.0f);
    vec3 wi = reflect(-wo, wm);
    float denom = abs(4.0f * wo.z * wi.z);
    if (denom < 0.0001f) return vec3(0.0f);
    float alpha = _RoughnessToAlpha(roughness);
    return D(wm, alpha) * G(wi, wo, alpha) * vec3(1.0f) / (4.0f * abs(wo.z * wi.z));
}

vec3 SpecularBRDF(in vec3 wi)
{
    return vec3(1.0f) / abs(wi.z);
}

#endif // MICROFACET_GLSL