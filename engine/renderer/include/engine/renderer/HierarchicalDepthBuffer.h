#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/MathTypes.h"
#include "engine/renderer/Camera.h"
#include <vector>

namespace Engine::Renderer {

struct HZBMip {
    Core::u32 width{0};
    Core::u32 height{0};
    std::vector<float> depths;
};

class HierarchicalDepthBuffer {
public:
    HierarchicalDepthBuffer() = default;
    ~HierarchicalDepthBuffer() = default;

    void BuildFromDepth(Core::u32 width, Core::u32 height, const float* depthData);
    void Clear();

    [[nodiscard]] bool IsAABBOccluded(const Core::AABB& aabb, const Camera& camera) const noexcept;

    [[nodiscard]] size_t GetMipCount() const noexcept { return m_mips.size(); }
    [[nodiscard]] const HZBMip* GetMip(size_t level) const noexcept {
        return (level < m_mips.size()) ? &m_mips[level] : nullptr;
    }

private:
    std::vector<HZBMip> m_mips;
};

} // namespace Engine::Renderer
