// =========================================================================
// Hierarchical Z-Buffer (HZB) Downsample Compute Shader
// Conservatively downsizes depth to mip pyramid for GPU occlusion culling
// =========================================================================

struct HZBParams {
    uint2 srcDimensions;
    uint2 dstDimensions;
};

ConstantBuffer<HZBParams> Params : register(b0);
Texture2D<float> SrcDepth : register(t0);
RWTexture2D<float> DstDepth : register(u0);

SamplerState PointSampler : register(s0);

[numthreads(16, 16, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
    if (dispatchThreadID.x >= Params.dstDimensions.x || dispatchThreadID.y >= Params.dstDimensions.y) {
        return;
    }

    uint2 srcCoord = dispatchThreadID.xy * 2;

    // Fetch 2x2 texels from previous mip level
    float d0 = SrcDepth.Load(int3(srcCoord + int2(0, 0), 0));
    float d1 = SrcDepth.Load(int3(min(srcCoord + int2(1, 0), Params.srcDimensions - 1), 0));
    float d2 = SrcDepth.Load(int3(min(srcCoord + int2(0, 1), Params.srcDimensions - 1), 0));
    float d3 = SrcDepth.Load(int3(min(srcCoord + int2(1, 1), Params.srcDimensions - 1), 0));

    // Conservative furthest depth for standard LH [0, 1] projection
    float maxDepth = max(max(d0, d1), max(d2, d3));

    DstDepth[dispatchThreadID.xy] = maxDepth;
}
