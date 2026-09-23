// =========================================================================
// GPU Compute Skinning Shader (Linear Blend Skinning - LBS)
// Shader Model 6.6
// =========================================================================

struct SkinnedVertexInput {
    float3 position;
    float3 normal;
    float4 tangent;
    float2 uv;
    uint4  boneIndices;
    float4 boneWeights;
};

struct SkinnedVertexOutput {
    float3 position;
    float3 normal;
    float4 tangent;
    float2 uv;
};

struct SkinningConstants {
    uint vertexCount;
    uint boneOffset;
    uint pad0[2];
};

ConstantBuffer<SkinningConstants> SkinningCB : register(b0);
StructuredBuffer<float4x4> BonePalette : register(t0);
StructuredBuffer<SkinnedVertexInput> InVertices : register(t1);

RWStructuredBuffer<SkinnedVertexOutput> OutVertices : register(u0);

[numthreads(64, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint vIdx = dispatchThreadID.x;
    if (vIdx >= SkinningCB.vertexCount) {
        return;
    }

    SkinnedVertexInput vin = InVertices[vIdx];

    // Normalize weights defensively
    float totalWeight = vin.boneWeights.x + vin.boneWeights.y + vin.boneWeights.z + vin.boneWeights.w;
    float4 weights = (totalWeight > 0.0001f) ? (vin.boneWeights / totalWeight) : float4(1.0f, 0.0f, 0.0f, 0.0f);

    float4x4 skinMatrix = 
        BonePalette[vin.boneIndices.x + SkinningCB.boneOffset] * weights.x +
        BonePalette[vin.boneIndices.y + SkinningCB.boneOffset] * weights.y +
        BonePalette[vin.boneIndices.z + SkinningCB.boneOffset] * weights.z +
        BonePalette[vin.boneIndices.w + SkinningCB.boneOffset] * weights.w;

    // Transform position and normal
    float3 skinnedPos = mul(float4(vin.position, 1.0f), skinMatrix).xyz;
    float3 skinnedNormal = normalize(mul(float4(vin.normal, 0.0f), skinMatrix).xyz);
    float3 skinnedTangent = normalize(mul(float4(vin.tangent.xyz, 0.0f), skinMatrix).xyz);

    SkinnedVertexOutput vout;
    vout.position = skinnedPos;
    vout.normal = skinnedNormal;
    vout.tangent = float4(skinnedTangent, vin.tangent.w);
    vout.uv = vin.uv;

    OutVertices[vIdx] = vout;
}
