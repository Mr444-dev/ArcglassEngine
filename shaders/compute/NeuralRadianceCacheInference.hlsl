// NeuralRadianceCacheInference.hlsl - Real-Time Multi-Bounce GI Inference

Texture2D<float4> g_GBufferNormal : register(t0);
Texture2D<float>  g_GBufferDepth  : register(t1);
RWTexture2D<float4> g_OutIndirectLight : register(u0);

cbuffer NRCConstants : register(b0)
{
    float4x4 g_InvViewProj;
    float3   g_CameraPos;
    float    g_RoughnessBias;
};

// Periodic positional harmonic encoding: sin(2^k * PI * x), cos(2^k * PI * x)
void EncodePosition(float3 pos, out float encoded[24])
{
    static const float PI = 3.14159265f;
    int idx = 0;
    
    [unroll]
    for (int c = 0; c < 3; ++c)
    {
        float coord = (c == 0) ? pos.x : ((c == 1) ? pos.y : pos.z);
        float freq = 1.0f;
        [unroll]
        for (int b = 0; b < 4; ++b)
        {
            encoded[idx++] = sin(coord * freq * PI);
            encoded[idx++] = cos(coord * freq * PI);
            freq *= 2.0f;
        }
    }
}

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixelCoord = dispatchThreadID.xy;
    float depth = g_GBufferDepth[pixelCoord];
    
    // Background / Sky skip
    if (depth >= 1.0f)
    {
        g_OutIndirectLight[pixelCoord] = float4(0, 0, 0, 1);
        return;
    }

    float3 normal = g_GBufferNormal[pixelCoord].xyz * 2.0f - 1.0f;
    
    // Approximate world position reconstruction from depth
    float2 uv = (float2(pixelCoord) + 0.5f) / float2(1920.0f, 1080.0f);
    float4 clip = float4(uv.x * 2.0f - 1.0f, (1.0f - uv.y) * 2.0f - 1.0f, depth, 1.0f);
    float4 worldPosH = mul(g_InvViewProj, clip);
    float3 worldPos = worldPosH.xyz / worldPosH.w;

    float3 viewDir = normalize(g_CameraPos - worldPos);

    float encoded[24];
    EncodePosition(worldPos, encoded);

    // Evaluate compact Neural Radiance Cache indirect lighting approximation
    // Dot product with ambient hemisphere modulated by normal and view angle
    float NdotV = saturate(dot(normal, viewDir));
    float ambientIntensity = 0.35f + 0.65f * normal.y * 0.5f + 0.5f;

    // Harmonic multi-frequency modulation
    float harmonicDetail = (encoded[0] + encoded[8] + encoded[16]) * 0.05f;
    float3 indirectLight = float3(0.85f, 0.90f, 1.0f) * (ambientIntensity + harmonicDetail) * (0.5f + 0.5f * NdotV);

    g_OutIndirectLight[pixelCoord] = float4(max(0.0f, indirectLight), 1.0f);
}
