// Small, texture-free viewport atmosphere and terrain helpers for the
// Arcglass ray-cast preview. These are editor preview effects, not the
// production RenderGraph sky/terrain passes.
float ArcHash21(float2 p) {
    p = frac(p * float2(123.34f, 456.21f));
    p += dot(p, p + 45.32f);
    return frac(p.x * p.y);
}

float ArcValueNoise(float2 p) {
    float2 i = floor(p);
    float2 f = frac(p);
    f = f * f * (3.0f - 2.0f * f);
    float a = ArcHash21(i);
    float b = ArcHash21(i + float2(1.0f, 0.0f));
    float c = ArcHash21(i + float2(0.0f, 1.0f));
    float d = ArcHash21(i + float2(1.0f, 1.0f));
    return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
}

float ArcCloudFbm(float2 p) {
    float value = ArcValueNoise(p) * 0.57f;
    value += ArcValueNoise(p * 2.03f + 17.1f) * 0.29f;
    value += ArcValueNoise(p * 4.07f - 9.2f) * 0.14f;
    return value;
}

float ArcTerrainHeight(float2 worldXZ) {
    float edge = smoothstep(6.0f, 12.0f, length(worldXZ));
    float broadHills = sin(worldXZ.x * 0.18f + cos(worldXZ.y * 0.12f)) * 1.15f;
    broadHills += cos(worldXZ.y * 0.23f - worldXZ.x * 0.08f) * 0.72f;
    broadHills += sin((worldXZ.x + worldXZ.y) * 0.11f) * 0.35f;
    return edge * broadHills;
}

float ArcTerrainShadow(float3 worldPosition, float3 lightDirection) {
    float visibility = lightDirection.y > 0.02f ? 1.0f : 0.55f;
    if (visibility > 0.55f) {
        [loop] for (int sampleIndex = 1; sampleIndex <= 5; ++sampleIndex) {
            float distanceAlongLight = float(sampleIndex) * 1.8f;
            float2 sampleXZ = worldPosition.xz + lightDirection.xz * distanceAlongLight;
            float rayHeight = worldPosition.y + lightDirection.y * distanceAlongLight;
            if (ArcTerrainHeight(sampleXZ) > rayHeight + 0.12f) {
                visibility = 0.48f;
                break;
            }
        }
    }
    return visibility;
}
float3 ArcTerrainNormal(float2 worldXZ) {
    const float epsilon = 0.16f;
    float left = ArcTerrainHeight(worldXZ - float2(epsilon, 0.0f));
    float right = ArcTerrainHeight(worldXZ + float2(epsilon, 0.0f));
    float back = ArcTerrainHeight(worldXZ - float2(0.0f, epsilon));
    float front = ArcTerrainHeight(worldXZ + float2(0.0f, epsilon));
    return normalize(float3(left - right, 2.0f * epsilon, back - front));
}

float3 ArcSkyRadiance(float3 origin, float3 direction, float3 sunDirection, float timeSeconds) {
    float horizon = saturate(direction.y * 2.2f + 0.18f);
    float3 sky = lerp(float3(0.30f, 0.43f, 0.60f), float3(0.055f, 0.12f, 0.25f), horizon);

    float sunCosine = saturate(dot(direction, sunDirection));
    float sunDisc = smoothstep(0.9992f, 0.99975f, sunCosine);
    float sunGlow = pow(sunCosine, 18.0f) * 0.20f;
    sky += float3(1.0f, 0.74f, 0.43f) * (sunDisc * 6.0f + sunGlow);

    if (direction.y > 0.035f) {
        const float cloudAltitude = 13.0f;
        float cloudDistance = (cloudAltitude - origin.y) / direction.y;
        if (cloudDistance > 0.0f && cloudDistance < 150.0f) {
            float2 cloudPosition = origin.xz + direction.xz * cloudDistance;
            cloudPosition = cloudPosition * 0.032f + float2(timeSeconds * 0.009f, -timeSeconds * 0.003f);
            float cloudDensity = smoothstep(0.48f, 0.71f, ArcCloudFbm(cloudPosition));
            float horizonFade = smoothstep(0.06f, 0.28f, direction.y);
            float3 cloudLit = lerp(float3(0.48f, 0.56f, 0.68f), float3(0.96f, 0.88f, 0.73f), saturate(sunDirection.y * 0.75f + 0.25f));
            sky = lerp(sky, cloudLit, cloudDensity * horizonFade * 0.90f);
        }
    }

    return sky;
}
