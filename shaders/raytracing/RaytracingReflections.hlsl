// Hardware Ray Tracing Shader (DirectX Raytracing DXR 1.1 / SM 6.6)

RaytracingAccelerationStructure g_SceneTLAS : register(t0, space0);
RWTexture2D<float4> g_OutputReflections : register(u0, space0);

struct RayPayload {
    float3 color;
    float hitT;
    float3 normal;
};

struct ShadowPayload {
    bool isHit;
};

// Ray Generation Shader
[shader("raygeneration")]
void RayGenMain() {
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchDimensions = DispatchRaysDimensions().xy;

    float2 uv = (float2(launchIndex) + 0.5f) / float2(launchDimensions);
    float2 ndc = uv * 2.0f - 1.0f;
    ndc.y = -ndc.y;

    RayDesc ray;
    ray.Origin = float3(0.0f, 2.0f, -5.0f);
    ray.Direction = normalize(float3(ndc.x, ndc.y, 1.0f));
    ray.TMin = 0.001f;
    ray.TMax = 1000.0f;

    RayPayload payload;
    payload.color = float3(0.0f, 0.0f, 0.0f);
    payload.hitT = -1.0f;
    payload.normal = float3(0.0f, 1.0f, 0.0f);

    TraceRay(
        g_SceneTLAS,
        RAY_FLAG_NONE,
        0xFF,
        0, // Hit group offset
        1, // Geometry stride
        0, // Miss index
        ray,
        payload
    );

    g_OutputReflections[launchIndex] = float4(payload.color, 1.0f);
}

// Closest Hit Shader for Primary Geometries
[shader("closesthit")]
void ClosestHitMain(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
    float3 barycentrics = float3(1.0f - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
    payload.color = barycentrics; // Visualize surface coordinates
    payload.hitT = RayTCurrent();
}

// Miss Shader (Procedural Sky)
[shader("miss")]
void MissMain(inout RayPayload payload) {
    float3 dir = WorldRayDirection();
    float t = 0.5f * (dir.y + 1.0f);
    payload.color = lerp(float3(1.0f, 1.0f, 1.0f), float3(0.5f, 0.7f, 1.0f), t);
    payload.hitT = -1.0f;
}

// Shadow Ray Miss Shader
[shader("miss")]
void ShadowMissMain(inout ShadowPayload payload) {
    payload.isHit = false;
}
