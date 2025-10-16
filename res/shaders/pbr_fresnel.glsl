#ifndef FRESNEL_GLSL
#define FRESNEL_GLSL
#include "pbr_common.glsl"

struct Complex
{
    float r; // real
    float i; // imaginary
};

struct Spectrum
{
    float r;
    float g;
    float b;
};

Complex _mul(in Complex a, in Complex b)
{
    return Complex(a.r * b.r - a.i * b.i, a.r * b.i + a.i * b.r);
}

Complex _div(in Complex a, in Complex b)
{
    float denom = b.r * b.r + b.i * b.i;
    return Complex((a.r * b.r + a.i * b.i) / denom, (a.i * b.r - a.r * b.i) / denom);
}

Complex _add(in Complex a, in Complex b)
{
    return Complex(a.r + b.r, a.i + b.i);
}

Complex _sub(in Complex a, in Complex b)
{
    return Complex(a.r - b.r, a.i - b.i);
}

Complex _sqrt(in Complex a)
{
    float magnitude = sqrt(sqrt(a.r * a.r + a.i * a.i));
    float angle = atan(a.i/ a.r) / 2.0f;
    return Complex(magnitude * cos(angle), magnitude * sin(angle));
}

float _norm(in Complex a)
{
    return (a.r * a.r + a.i * a.i);
}

float _GetEta(in float IORi, in float IORt)
{
    return IORt / IORi;
}

Complex _GetEta(in Complex IORi, in Complex IORt)
{
    return _div(IORt, IORi);
}

float Fresnel(float cosThetaI, float IORi, float IORt)
{
    cosThetaI = clamp(cosThetaI, -1.0f, 1.0f);
    float eta = _GetEta(IORi, IORt);
    if (cosThetaI < 0)
    {
        eta = 1.0f / eta;
        cosThetaI = -cosThetaI;
    }

    // snell's law
    float sin2ThetaT = (1.0f - cosThetaI * cosThetaI) / (eta * eta);
    if (sin2ThetaT >= 1.0f)
    {
        // total internal reflection
        return 1.0f;
    }
    float cosThetaT = sqrt(1.0f - sin2ThetaT);

    float r_parl = (eta * cosThetaI - cosThetaT) / (eta * cosThetaI + cosThetaT);
    float r_perp = (cosThetaI - eta * cosThetaT) / (cosThetaI + eta * cosThetaT);

    return (r_parl * r_parl + r_perp * r_perp) * 0.5f;
}

float Fresnel(float cosThetaI, in Complex IORi, in Complex IORt)
{
    cosThetaI = clamp(cosThetaI, 0.0f, 1.0f);
    Complex eta = _GetEta(IORi, IORt);

    // snell's law
    float sin2ThetaI = 1 - cosThetaI * cosThetaI;
    Complex sin2ThetaT = _div(Complex(sin2ThetaI, 0), _mul(eta, eta));
    Complex cosThetaT = _sqrt(_sub(Complex(1.0f, 0), sin2ThetaT));

    Complex r_parl = _div(
        _sub(_mul(eta, Complex(cosThetaI, 0)), cosThetaT),
        _add(_mul(eta, Complex(cosThetaI, 0)), cosThetaT)
    );

    Complex r_perp = _div(
        _sub(Complex(cosThetaI, 0), _mul(eta, cosThetaT)),
        _add(Complex(cosThetaI, 0), _mul(eta, cosThetaT))
    );

    return (_norm(r_parl) + _norm(r_perp)) * 0.5f;
}

#endif // FRESNEL_GLSL