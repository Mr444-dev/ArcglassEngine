#include "engine/core/BaseTypes.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "engine/renderer/passes/VolumetricFogPass.h"
#include "engine/renderer/passes/TAAPass.h"
#include "engine/renderer/RenderGraph.h"

void TestVolumetricFogFroxelCalculations() {
    using namespace Engine::Renderer;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Volumetric Fog Froxel Calculations ---");

    constexpr u32 SLICES = 64;
    constexpr float NEAR_Z = 0.1f;
    constexpr float FAR_Z = 100.0f;

    float prevDepth = 0.0f;
    for (u32 s = 0; s < SLICES; ++s) {
        float depth = VolumetricFogPass::CalculateSliceDepth(s, SLICES, NEAR_Z, FAR_Z);
        ENGINE_ASSERT(depth >= NEAR_Z && depth <= FAR_Z, "Froxel slice depth out of bounds");
        ENGINE_ASSERT(depth > prevDepth, "Froxel slice depth must be strictly monotonically increasing");
        prevDepth = depth;
    }

    // Henyey-Greenstein phase evaluation
    const float forwardPhase = VolumetricFogPass::EvaluatePhase(1.0f, 0.5f);
    const float backwardPhase = VolumetricFogPass::EvaluatePhase(-1.0f, 0.5f);
    ENGINE_ASSERT(forwardPhase > backwardPhase, "Forward scattering phase must exceed backscattering for g > 0");
    ENGINE_ASSERT(forwardPhase > 0.0f && backwardPhase > 0.0f, "Phase function values must be positive");

    ENGINE_LOG_INFO("Volumetric Fog Froxel depth and Henyey-Greenstein phase verified (Forward: {:.4f}, Backward: {:.4f}).",
        forwardPhase, backwardPhase);
}

void TestTAAJitterAndVarianceClipping() {
    using namespace Engine::Renderer;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing TAA Halton Jitter & Color Variance Clipping ---");

    // 1. Verify 16 Halton samples
    for (u32 i = 1; i <= 16; ++i) {
        float h2 = TAAPass::GetHalton(i, 2);
        float h3 = TAAPass::GetHalton(i, 3);
        ENGINE_ASSERT(h2 >= 0.0f && h2 < 1.0f, "Halton base 2 out of range [0, 1)");
        ENGINE_ASSERT(h3 >= 0.0f && h3 < 1.0f, "Halton base 3 out of range [0, 1)");
    }

    // 2. Subpixel jitter bounds for 1920x1080
    Vec2 jitter = TAAPass::GetJitter(0, 1920, 1080);
    ENGINE_ASSERT(std::abs(jitter.x) <= 2.0f / 1920.0f, "Jitter X exceeds subpixel bounds");
    ENGINE_ASSERT(std::abs(jitter.y) <= 2.0f / 1080.0f, "Jitter Y exceeds subpixel bounds");

    // 3. Variance clipping
    Vec3 mean{0.5f, 0.5f, 0.5f};
    Vec3 stdDev{0.1f, 0.1f, 0.1f};

    // Sample inside bounds should remain unchanged
    Vec3 insideSample{0.55f, 0.45f, 0.52f};
    Vec3 clippedInside = TAAPass::ClipColor(insideSample, mean, stdDev, 1.0f);
    ENGINE_ASSERT(std::abs(clippedInside.x - insideSample.x) < 1e-4f, "Inside sample must not be clipped");

    // Sample outside bounds (ghosting artifact) should be clamped to [0.4, 0.6]
    Vec3 outlierSample{0.9f, 0.1f, 0.7f};
    Vec3 clippedOutlier = TAAPass::ClipColor(outlierSample, mean, stdDev, 1.0f);
    ENGINE_ASSERT(clippedOutlier.x <= 0.6001f && clippedOutlier.x >= 0.5999f, "Outlier X must be clamped to max bound");
    ENGINE_ASSERT(clippedOutlier.y <= 0.4001f && clippedOutlier.y >= 0.3999f, "Outlier Y must be clamped to min bound");

    ENGINE_LOG_INFO("TAA Halton jitter and Color Variance Clipping verified.");
}

void TestNextGenRenderGraphIntegration() {
    using namespace Engine::Renderer;
    using namespace Engine::Core;
    using namespace Engine::RHI;

    Logger::LogMessage(LogLevel::Info, "--- Milestone Phase 12: Next-Gen RenderGraph Integration ---");

    RenderGraph rg;

    RGTextureDesc depthDesc{1280, 720, Format::D32_FLOAT, TextureUsage::DepthStencil, "TransientDepth"};
    RGTextureHandle depth = rg.DeclareTexture("TransientDepth", depthDesc);

    RGTextureDesc fogDesc{160, 90, Format::RGBA16_FLOAT, TextureUsage::ShaderResource | TextureUsage::UnorderedAccess, "FroxelFogVolume"};
    RGTextureHandle fogVolume = rg.DeclareTexture("FroxelFogVolume", fogDesc);

    RGTextureDesc colorDesc{1280, 720, Format::RGBA16_FLOAT, TextureUsage::RenderTarget | TextureUsage::ShaderResource, "HDRSceneColor"};
    RGTextureHandle currentColor = rg.DeclareTexture("HDRSceneColor", colorDesc);

    RGTextureDesc historyDesc{1280, 720, Format::RGBA16_FLOAT, TextureUsage::ShaderResource, "TAAHistory"};
    RGTextureHandle history = rg.DeclareTexture("TAAHistory", historyDesc);

    RGTextureDesc resolvedDesc{1280, 720, Format::RGBA16_FLOAT, TextureUsage::RenderTarget | TextureUsage::ShaderResource, "TAAResolved"};
    RGTextureHandle resolved = rg.DeclareTexture("TAAResolved", resolvedDesc);

    // Schedule passes
    VolumetricFogPass::AddToGraph(rg, depth, fogVolume);
    TAAPass::AddToGraph(rg, currentColor, history, resolved);
    rg.MarkOutput(resolved);
    rg.MarkOutput(fogVolume);

    ENGINE_ASSERT(rg.Compile(), "Next-Gen RenderGraph compilation failed");
    ENGINE_ASSERT(rg.GetCompiledPassCount() == 2, "Expected 2 active next-gen passes scheduled");

    ENGINE_LOG_INFO("Next-Gen RenderGraph successfully scheduled VolumetricFogPass and TAAPass.");
}

void RunNextGenRendererUnitTests() {
    TestVolumetricFogFroxelCalculations();
    TestTAAJitterAndVarianceClipping();
    TestNextGenRenderGraphIntegration();
}
