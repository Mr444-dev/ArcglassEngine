// TerrainClipmapUpdate.hlsl - GPU-Driven Continuous Clipmap & Geomorphing

struct TerrainVertex
{
    float3 position;
    float3 normal;
    float2 uv;
    float  morphFactor;
};

RWStructuredBuffer<TerrainVertex> g_OutVertices : register(u0);

cbuffer ClipmapParams : register(b0)
{
    float2 g_RingCenter;
    float  g_GridSpacing;
    float  g_InnerRadius;
    float  g_OuterRadius;
    uint   g_GridResolution;
};

float SampleHeight(float2 worldXZ)
{
    float o1 = sin(worldXZ.x * 0.015f) * cos(worldXZ.y * 0.015f) * 25.0f;
    float o2 = sin(worldXZ.x * 0.05f) * cos(worldXZ.y * 0.05f) * 8.0f;
    return o1 + o2;
}

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint x = dispatchThreadID.x;
    uint z = dispatchThreadID.y;
    if (x >= g_GridResolution || z >= g_GridResolution) return;

    uint index = z * g_GridResolution + x;

    // Compute localized grid position snapped to ring center
    float2 localOffset = (float2(x, z) - (float(g_GridResolution) * 0.5f)) * g_GridSpacing;
    float2 worldXZ = g_RingCenter + localOffset;

    float distToCenter = length(localOffset);
    float alpha = saturate((distToCenter - g_InnerRadius) / max(0.001f, g_OuterRadius - g_InnerRadius));

    // Sample height for current level
    float h = SampleHeight(worldXZ);

    // Sample coarse height for adjacent outer level to perform geomorphing
    float2 coarseSnapped = floor(worldXZ / (g_GridSpacing * 2.0f)) * (g_GridSpacing * 2.0f);
    float coarseHeight = SampleHeight(coarseSnapped);

    // Continuous height interpolation: eliminate vertex popping between rings
    float finalHeight = lerp(h, coarseHeight, alpha);

    // Analytical finite difference for normal calculation
    float hL = SampleHeight(worldXZ - float2(g_GridSpacing, 0));
    float hR = SampleHeight(worldXZ + float2(g_GridSpacing, 0));
    float hD = SampleHeight(worldXZ - float2(0, g_GridSpacing));
    float hU = SampleHeight(worldXZ + float2(0, g_GridSpacing));

    float3 normal = normalize(float3(hL - hR, 2.0f * g_GridSpacing, hD - hU));

    TerrainVertex vert;
    vert.position = float3(worldXZ.x, finalHeight, worldXZ.y);
    vert.normal = normal;
    vert.uv = worldXZ * 0.05f;
    vert.morphFactor = alpha;

    g_OutVertices[index] = vert;
}
