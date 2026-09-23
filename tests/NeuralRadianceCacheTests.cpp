#include "engine/core/BaseTypes.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "engine/renderer/NeuralRadianceCache.h"
#include "engine/renderer/RenderGraph.h"

void TestSpatialFrequencyEncoding() {
    using namespace Engine::Core;
    using namespace Engine::Renderer;

    Logger::LogMessage(LogLevel::Info, "--- Testing Spatial Harmonic Frequency Encoding (NRC) ---");

    const Vec3 pos{1.0f, 0.5f, -2.0f};
    const auto encoded = NeuralRadianceCache::EncodePosition(pos);

    ENGINE_ASSERT(encoded.size() == NeuralRadianceCache::ENCODED_POS_DIM, "Encoded dimension mismatch");

    // Verify sin^2 + cos^2 = 1.0 for each harmonic frequency pair
    for (size_t i = 0; i < encoded.size(); i += 2) {
        const float s = encoded[i];
        const float c = encoded[i + 1];
        const float sumSquares = (s * s) + (c * c);
        ENGINE_ASSERT(std::abs(sumSquares - 1.0f) < 0.001f, "Harmonic sin^2 + cos^2 identity failed");
    }

    ENGINE_LOG_INFO("Spatial frequency encoding verified (24-dim harmonic features, sin^2+cos^2=1).");
}

void TestNRCInferencePrediction() {
    using namespace Engine::Core;
    using namespace Engine::Renderer;

    Logger::LogMessage(LogLevel::Info, "--- Testing Neural Radiance Cache Forward Inference ---");

    NeuralRadianceCache nrc;

    NRCInputQuery query;
    query.worldPos = {10.0f, 2.0f, 5.0f};
    query.normal = {0.0f, 1.0f, 0.0f}; // Upward surface
    query.viewDir = {0.0f, -0.707f, 0.707f};
    query.roughness = 0.2f;
    query.albedo = {0.9f, 0.9f, 0.9f};

    const Vec3 radiance = nrc.QueryRadiance(query);

    // Radiance must be strictly non-negative (physical realism constraint)
    ENGINE_ASSERT(radiance.x >= 0.0f, "Red radiance cannot be negative");
    ENGINE_ASSERT(radiance.y >= 0.0f, "Green radiance cannot be negative");
    ENGINE_ASSERT(radiance.z >= 0.0f, "Blue radiance cannot be negative");

    ENGINE_LOG_INFO("NRC predicted indirect radiance: [{:.3f}, {:.3f}, {:.3f}].",
        radiance.x, radiance.y, radiance.z);
}

void TestNRCOnlineTrainingConvergence() {
    using namespace Engine::Core;
    using namespace Engine::Renderer;

    Logger::LogMessage(LogLevel::Info, "--- Milestone Phase 16: NRC Online Training Convergence ---");

    NeuralRadianceCache nrc;

    NRCTrainingSample sample;
    sample.query.worldPos = {0.0f, 1.0f, 0.0f};
    sample.query.normal = {0.0f, 1.0f, 0.0f};
    sample.query.viewDir = {0.0f, -1.0f, 0.0f};
    sample.query.roughness = 0.5f;
    sample.targetRadiance = {0.8f, 0.6f, 0.4f}; // Warm bounce light target

    const float initialLoss = nrc.TrainStep(sample, 0.05f);

    // Train for 50 SGD steps
    float finalLoss = initialLoss;
    for (int i = 0; i < 50; ++i) {
        finalLoss = nrc.TrainStep(sample, 0.05f);
    }

    ENGINE_ASSERT(finalLoss < initialLoss, "Training loss must decrease");
    ENGINE_ASSERT(nrc.GetTotalTrainingSteps() == 51, "Step count mismatch");

    const Vec3 trainedPred = nrc.QueryRadiance(sample.query);

    ENGINE_LOG_INFO("NRC online training converged: Loss decreased from {:.4f} to {:.4f} (Prediction: [{:.3f}, {:.3f}, {:.3f}], Target: [{:.1f}, {:.1f}, {:.1f}]).",
        initialLoss, finalLoss, trainedPred.x, trainedPred.y, trainedPred.z,
        sample.targetRadiance.x, sample.targetRadiance.y, sample.targetRadiance.z);
}

void TestNRCRenderGraphIntegration() {
    using namespace Engine::Renderer;
    using namespace Engine::RHI;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Neural Radiance Cache RenderGraph Integration ---");

    RenderGraph rg;

    RGTextureDesc normalDesc{1280, 720, Format::RGBA16_FLOAT, TextureUsage::RenderTarget | TextureUsage::ShaderResource, "GBufferNormal"};
    RGTextureHandle normal = rg.DeclareTexture("GBufferNormal", normalDesc);

    RGTextureDesc depthDesc{1280, 720, Format::D32_FLOAT, TextureUsage::DepthStencil, "GBufferDepth"};
    RGTextureHandle depth = rg.DeclareTexture("GBufferDepth", depthDesc);

    RGTextureDesc indirectDesc{1280, 720, Format::RGBA16_FLOAT, TextureUsage::RenderTarget | TextureUsage::ShaderResource, "NRCIndirectLight"};
    RGTextureHandle indirect = rg.DeclareTexture("NRCIndirectLight", indirectDesc);

    NeuralRadianceCache nrc;
    nrc.AddToGraph(rg, normal, depth, indirect);
    rg.MarkOutput(indirect);

    ENGINE_ASSERT(rg.Compile(), "RenderGraph compilation with NRC pass must succeed");
    ENGINE_ASSERT(rg.GetCompiledPassCount() == 1, "Expected 1 active NRC pass scheduled");

    ENGINE_LOG_INFO("Neural Radiance Cache RenderGraph pass scheduled and validated.");
}

void RunNeuralRadianceCacheUnitTests() {
    TestSpatialFrequencyEncoding();
    TestNRCInferencePrediction();
    TestNRCOnlineTrainingConvergence();
    TestNRCRenderGraphIntegration();
}
