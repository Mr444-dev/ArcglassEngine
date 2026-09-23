// Temporal Anti-Aliasing (TAA) Shader with Variance Clipping (SM 6.6)
#define TAA_THREADS_X 8
#define TAA_THREADS_Y 8

struct TAAConstants {
    float2 jitterOffset;
    float2 renderTargetSize;
    float historyBlendWeight; // Typically 0.9
    float varianceClipGamma;  // Typically 1.0 - 1.25
    float2 pad;
};

// RGB to YCoCg color space for perceptually uniform variance clipping
float3 RGBToYCoCg(float3 rgb) {
    float y  =  0.25f * rgb.r + 0.5f * rgb.g + 0.25f * rgb.b;
    float co =  0.5f  * rgb.r                - 0.5f  * rgb.b;
    float cg = -0.25f * rgb.r + 0.5f * rgb.g - 0.25f * rgb.b;
    return float3(y, co, cg);
}

float3 YCoCgToRGB(float3 ycocg) {
    float r = ycocg.x + ycocg.y - ycocg.z;
    float g = ycocg.x           + ycocg.z;
    float b = ycocg.x - ycocg.y - ycocg.z;
    return float3(r, g, b);
}

// Clip history color to local 3x3 neighborhood bounding box
float3 ClipToAABB(float3 historySample, float3 aabbMin, float3 aabbMax) {
    return clamp(historySample, aabbMin, aabbMax);
}

[numthreads(TAA_THREADS_X, TAA_THREADS_Y, 1)]
void CSMain(uint2 dispatchThreadID : SV_DispatchThreadID) {
    // TAA resolves current jittered frame against reprojection history buffer
}
