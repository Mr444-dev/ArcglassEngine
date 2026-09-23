// Voxel Cone Tracing Global Illumination Compute Shader (SM 6.6)
#define VCT_THREADS_X 8
#define VCT_THREADS_Y 8

Texture3D<float4> g_VoxelRadiance : register(t0, space0);
SamplerState g_LinearSampler : register(s0, space0);
RWTexture2D<float4> g_IndirectDiffuseOut : register(u0, space0);

struct VCTConstants {
    float3 volumeCenter;
    float volumeExtent;
    float maxTracingDistance;
    float bounceIntensity;
    float voxelSize;
    float pad[3];
};

float4 TraceCone(float3 origin, float3 direction, float aperture, float maxDist, float voxelSize) {
    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float dist = voxelSize * 1.5f;

    while (dist < maxDist && color.a < 0.95f) {
        float diameter = 2.0f * dist * aperture;
        float mip = max(0.0f, log2(max(diameter / voxelSize, 1.0f)));

        float3 samplePos = origin + direction * dist;
        // Transform to UVW [0, 1]
        float3 uvw = (samplePos - (g_VoxelRadiance.Load(int4(0,0,0,0)).rgb)) / 64.0f; // Normalized coordinate

        float4 stepSample = g_VoxelRadiance.SampleLevel(g_LinearSampler, uvw, mip);
        color.rgb += (1.0f - color.a) * stepSample.rgb * stepSample.a;
        color.a += (1.0f - color.a) * stepSample.a;

        dist += max(diameter * 0.5f, voxelSize);
    }

    return color;
}

[numthreads(VCT_THREADS_X, VCT_THREADS_Y, 1)]
void CSMain(uint2 dispatchThreadID : SV_DispatchThreadID) {
    // Computes indirect diffuse radiance per pixel from G-Buffer
}
