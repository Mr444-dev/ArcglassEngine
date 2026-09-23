#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/MathTypes.h"
#include "engine/renderer/RenderGraph.h"
#include <array>

namespace Engine::Renderer {

struct NRCInputQuery {
    Core::Vec3 worldPos{0.0f, 0.0f, 0.0f};
    Core::Vec3 normal{0.0f, 1.0f, 0.0f};
    Core::Vec3 viewDir{0.0f, 0.0f, 1.0f};
    float roughness{0.5f};
    Core::Vec3 albedo{0.8f, 0.8f, 0.8f};
};

struct NRCTrainingSample {
    NRCInputQuery query;
    Core::Vec3 targetRadiance{0.0f, 0.0f, 0.0f};
};

class NeuralRadianceCache {
public:
    static constexpr size_t NUM_FREQUENCY_BANDS = 4;
    static constexpr size_t ENCODED_POS_DIM = 3 * NUM_FREQUENCY_BANDS * 2; // 24 floats
    static constexpr size_t INPUT_FEATURE_DIM = ENCODED_POS_DIM + 3 + 3 + 1; // 31 floats
    static constexpr size_t HIDDEN_DIM = 16;
    static constexpr size_t OUTPUT_DIM = 3; // RGB Radiance

    NeuralRadianceCache();

    // Multi-resolution periodic frequency encoding: sin(2^k * pi * x), cos(2^k * pi * x)
    [[nodiscard]] static std::array<float, ENCODED_POS_DIM> EncodePosition(const Core::Vec3& pos) noexcept;

    // Evaluates indirect multi-bounce radiance from the neural cache
    [[nodiscard]] Core::Vec3 QueryRadiance(const NRCInputQuery& query) const noexcept;

    // Executes an online stochastic gradient descent step using sample radiance
    float TrainStep(const NRCTrainingSample& sample, float learningRate = 0.02f) noexcept;

    // Batched inference across deferred screen pixels
    void AddToGraph(
        RenderGraph& rg,
        RGTextureHandle gBufferNormal,
        RGTextureHandle gBufferDepth,
        RGTextureHandle outIndirectLight
    );

    [[nodiscard]] Core::u64 GetTotalTrainingSteps() const noexcept { return m_totalSteps; }
    [[nodiscard]] float GetAverageLoss() const noexcept { return m_runningLoss; }

private:
    // Compact 2-layer MLP weights
    std::array<float, INPUT_FEATURE_DIM * HIDDEN_DIM> m_weightsLayer1{};
    std::array<float, HIDDEN_DIM> m_biasLayer1{};

    std::array<float, HIDDEN_DIM * OUTPUT_DIM> m_weightsLayer2{};
    std::array<float, OUTPUT_DIM> m_biasLayer2{};

    Core::u64 m_totalSteps{0};
    float m_runningLoss{0.0f};
};

} // namespace Engine::Renderer
