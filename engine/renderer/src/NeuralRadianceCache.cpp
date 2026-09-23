#include "engine/renderer/NeuralRadianceCache.h"
#include <cmath>
#include <numbers>
#include <algorithm>

namespace Engine::Renderer {

NeuralRadianceCache::NeuralRadianceCache() {
    // Initialize weights with deterministic Xavier/Glorot normal distribution
    constexpr float scale1 = 0.2f;
    for (size_t i = 0; i < m_weightsLayer1.size(); ++i) {
        m_weightsLayer1[i] = std::sin(static_cast<float>(i) * 0.17f) * scale1;
    }
    m_biasLayer1.fill(0.01f);

    constexpr float scale2 = 0.25f;
    for (size_t i = 0; i < m_weightsLayer2.size(); ++i) {
        m_weightsLayer2[i] = std::cos(static_cast<float>(i) * 0.23f) * scale2;
    }
    m_biasLayer2.fill(0.05f);
}

std::array<float, NeuralRadianceCache::ENCODED_POS_DIM> NeuralRadianceCache::EncodePosition(const Core::Vec3& pos) noexcept {
    std::array<float, ENCODED_POS_DIM> encoded{};
    const float coords[3] = {pos.x, pos.y, pos.z};

    size_t outIdx = 0;
    for (int c = 0; c < 3; ++c) {
        float freq = 1.0f;
        for (size_t b = 0; b < NUM_FREQUENCY_BANDS; ++b) {
            const float angle = coords[c] * freq * std::numbers::pi_v<float>;
            encoded[outIdx++] = std::sin(angle);
            encoded[outIdx++] = std::cos(angle);
            freq *= 2.0f;
        }
    }

    return encoded;
}

Core::Vec3 NeuralRadianceCache::QueryRadiance(const NRCInputQuery& query) const noexcept {
    // 1. Construct feature vector (Position harmonics + Normal + ViewDir + Roughness)
    std::array<float, INPUT_FEATURE_DIM> features{};
    const auto encodedPos = EncodePosition(query.worldPos);
    std::copy(encodedPos.begin(), encodedPos.end(), features.begin());

    size_t idx = ENCODED_POS_DIM;
    features[idx++] = query.normal.x;
    features[idx++] = query.normal.y;
    features[idx++] = query.normal.z;

    features[idx++] = query.viewDir.x;
    features[idx++] = query.viewDir.y;
    features[idx++] = query.viewDir.z;

    features[idx++] = query.roughness;

    // 2. Hidden Layer (Matrix multiply + ReLU)
    std::array<float, HIDDEN_DIM> hidden{};
    for (size_t h = 0; h < HIDDEN_DIM; ++h) {
        float sum = m_biasLayer1[h];
        for (size_t f = 0; f < INPUT_FEATURE_DIM; ++f) {
            sum += features[f] * m_weightsLayer1[f * HIDDEN_DIM + h];
        }
        hidden[h] = std::max(0.0f, sum); // ReLU activation
    }

    // 3. Output Layer (Matrix multiply + Non-negative clamp)
    float rgb[3] = {0.0f, 0.0f, 0.0f};
    for (size_t o = 0; o < OUTPUT_DIM; ++o) {
        float sum = m_biasLayer2[o];
        for (size_t h = 0; h < HIDDEN_DIM; ++h) {
            sum += hidden[h] * m_weightsLayer2[h * OUTPUT_DIM + o];
        }
        rgb[o] = std::max(0.0f, sum); // Radiance is strictly non-negative
    }

    return {rgb[0], rgb[1], rgb[2]};
}

float NeuralRadianceCache::TrainStep(const NRCTrainingSample& sample, float learningRate) noexcept {
    // Forward Pass
    std::array<float, INPUT_FEATURE_DIM> features{};
    const auto encodedPos = EncodePosition(sample.query.worldPos);
    std::copy(encodedPos.begin(), encodedPos.end(), features.begin());

    size_t idx = ENCODED_POS_DIM;
    features[idx++] = sample.query.normal.x;
    features[idx++] = sample.query.normal.y;
    features[idx++] = sample.query.normal.z;
    features[idx++] = sample.query.viewDir.x;
    features[idx++] = sample.query.viewDir.y;
    features[idx++] = sample.query.viewDir.z;
    features[idx++] = sample.query.roughness;

    std::array<float, HIDDEN_DIM> hidden{};
    for (size_t h = 0; h < HIDDEN_DIM; ++h) {
        float sum = m_biasLayer1[h];
        for (size_t f = 0; f < INPUT_FEATURE_DIM; ++f) {
            sum += features[f] * m_weightsLayer1[f * HIDDEN_DIM + h];
        }
        hidden[h] = std::max(0.0f, sum);
    }

    float predRgb[3] = {0.0f, 0.0f, 0.0f};
    for (size_t o = 0; o < OUTPUT_DIM; ++o) {
        float sum = m_biasLayer2[o];
        for (size_t h = 0; h < HIDDEN_DIM; ++h) {
            sum += hidden[h] * m_weightsLayer2[h * OUTPUT_DIM + o];
        }
        predRgb[o] = sum;
    }

    const float targetRgb[3] = {
        sample.targetRadiance.x,
        sample.targetRadiance.y,
        sample.targetRadiance.z
    };

    // Loss: 0.5 * sum((pred - target)^2)
    float dOut[3] = {0.0f, 0.0f, 0.0f};
    float totalLoss = 0.0f;
    for (size_t o = 0; o < 3; ++o) {
        const float diff = predRgb[o] - targetRgb[o];
        totalLoss += 0.5f * diff * diff;
        dOut[o] = diff;
    }

    // Backprop to Layer 2
    std::array<float, HIDDEN_DIM> dHidden{};
    for (size_t h = 0; h < HIDDEN_DIM; ++h) {
        for (size_t o = 0; o < OUTPUT_DIM; ++o) {
            dHidden[h] += dOut[o] * m_weightsLayer2[h * OUTPUT_DIM + o];
            m_weightsLayer2[h * OUTPUT_DIM + o] -= learningRate * dOut[o] * hidden[h];
        }
    }
    for (size_t o = 0; o < OUTPUT_DIM; ++o) {
        m_biasLayer2[o] -= learningRate * dOut[o];
    }

    // Backprop to Layer 1 (through ReLU)
    for (size_t h = 0; h < HIDDEN_DIM; ++h) {
        if (hidden[h] <= 0.0f) continue; // ReLU derivative is 0
        for (size_t f = 0; f < INPUT_FEATURE_DIM; ++f) {
            m_weightsLayer1[f * HIDDEN_DIM + h] -= learningRate * dHidden[h] * features[f];
        }
        m_biasLayer1[h] -= learningRate * dHidden[h];
    }

    m_totalSteps++;
    m_runningLoss = (m_runningLoss * 0.95f) + (totalLoss * 0.05f);

    return totalLoss;
}

void NeuralRadianceCache::AddToGraph(
    RenderGraph& rg,
    RGTextureHandle gBufferNormal,
    RGTextureHandle gBufferDepth,
    RGTextureHandle outIndirectLight) {
    rg.AddPass("NeuralRadianceCachePass",
        [&](RGPassBuilder& builder) {
            builder.ReadTexture(gBufferNormal, RHI::ResourceState::ShaderResource);
            builder.ReadTexture(gBufferDepth, RHI::ResourceState::ShaderResource);
            builder.WriteTexture(outIndirectLight, RHI::ResourceState::RenderTarget);
        },
        [](RHI::RHICommandList& cmdList, const RenderGraph& graph) {
            (void)cmdList;
            (void)graph;
        }
    );
}

} // namespace Engine::Renderer
