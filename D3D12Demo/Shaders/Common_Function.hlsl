#ifndef _COMMON_FUNCTION_H_
#define _COMMON_FUNCTION_H_

float GammaCorrection(float a)
{
    if (a > 0.0031308f)
    {
        return 1.055f * pow(abs(a), 1.0f / 2.4f) - 0.055f;
    }
    else
    {
        return 12.92f * a;
    }
}

float3 linearColorToSRGB(float3 color)
{
    return float3(GammaCorrection(color.r), GammaCorrection(color.g), GammaCorrection(color.b));
}

#endif