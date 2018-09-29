

float DistanceFallOff(float refDist, float d)
{
    float distDelta = refDist / max(d, 0.01f);
    float distFalloff = distDelta * distDelta;
    return distFalloff;
}

float DistanceWindowFunction(float maxRadius, float d)
{
    float winDelta = clamp(1.0f - pow(d / maxRadius, 4.0f), 0.0f, 1.0f);
    return winDelta * winDelta;
}

float PointLightFallOff(LightInfo light, float toLightLen)
{
    float distFalloff = DistanceFallOff(light.RefDist, toLightLen);
    float winFallOff = DistanceWindowFunction(light.MaxRadius, toLightLen);
    
    return distFalloff * winFallOff;
}

float SpotLightFallOff(LightInfo light, float toLightLen, float3 toLight)
{
    float distFalloff = DistanceFallOff(light.RefDist, toLightLen);
    float winFallOff = DistanceWindowFunction(light.MaxRadius, toLightLen);
  
    float dirDelta = (dot(toLight, -light.LightDirection.xyz) - light.MaxAngle) / (light.MinAngle - light.MaxAngle);
    dirDelta = clamp(dirDelta, 0.0f, 1.0f);
    float dirFalloff = pow(dirDelta, 2.0f);

    return distFalloff * dirFalloff;
}
