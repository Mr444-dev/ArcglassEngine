#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/MathTypes.h"
#include "engine/renderer/Camera.h"
#include <array>

namespace Engine::Renderer {

struct ShadowCascade {
    float splitDistance{0.0f};
    Core::Mat4x4 lightViewProj{Core::Mat4x4::Identity()};
};

class CascadedShadowMaps {
public:
    static constexpr size_t CASCADE_COUNT = 4;

    CascadedShadowMaps();
    ~CascadedShadowMaps() = default;

    void Update(
        const Camera& camera,
        const Core::Vec3& lightDirection,
        float maxShadowDistance = 150.0f,
        float splitLambda = 0.85f
    );

    [[nodiscard]] const ShadowCascade& GetCascade(size_t index) const noexcept {
        return m_cascades[index];
    }

    [[nodiscard]] size_t GetCascadeCount() const noexcept { return CASCADE_COUNT; }
    [[nodiscard]] Core::u32 GetResolution() const noexcept { return m_resolution; }

private:
    std::array<ShadowCascade, CASCADE_COUNT> m_cascades;
    Core::u32 m_resolution{2048};
};

} // namespace Engine::Renderer
