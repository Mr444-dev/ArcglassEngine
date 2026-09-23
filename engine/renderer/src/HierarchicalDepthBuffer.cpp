#include "engine/renderer/HierarchicalDepthBuffer.h"
#include <algorithm>
#include <cmath>

namespace Engine::Renderer {

void HierarchicalDepthBuffer::Clear() {
    m_mips.clear();
}

void HierarchicalDepthBuffer::BuildFromDepth(Core::u32 width, Core::u32 height, const float* depthData) {
    Clear();
    if (width == 0 || height == 0 || !depthData) return;

    // Mip 0 (base resolution)
    HZBMip mip0{};
    mip0.width = width;
    mip0.height = height;
    mip0.depths.assign(depthData, depthData + width * height);
    m_mips.push_back(std::move(mip0));

    Core::u32 curW = width;
    Core::u32 curH = height;

    while (curW > 1 || curH > 1) {
        const Core::u32 nextW = std::max(1u, curW / 2);
        const Core::u32 nextH = std::max(1u, curH / 2);
        const auto& prevDepths = m_mips.back().depths;

        HZBMip nextMip{};
        nextMip.width = nextW;
        nextMip.height = nextH;
        nextMip.depths.resize(nextW * nextH);

        for (Core::u32 y = 0; y < nextH; ++y) {
            for (Core::u32 x = 0; x < nextW; ++x) {
                const Core::u32 x0 = x * 2;
                const Core::u32 x1 = std::min(x0 + 1, curW - 1);
                const Core::u32 y0 = y * 2;
                const Core::u32 y1 = std::min(y0 + 1, curH - 1);

                const float d0 = prevDepths[y0 * curW + x0];
                const float d1 = prevDepths[y0 * curW + x1];
                const float d2 = prevDepths[y1 * curW + x0];
                const float d3 = prevDepths[y1 * curW + x1];

                // Maximum depth reduction (conservative occluder)
                nextMip.depths[y * nextW + x] = std::max({d0, d1, d2, d3});
            }
        }

        m_mips.push_back(std::move(nextMip));
        curW = nextW;
        curH = nextH;
    }
}

bool HierarchicalDepthBuffer::IsAABBOccluded(const Core::AABB& aabb, const Camera& camera) const noexcept {
    if (m_mips.empty()) return false;

    // Project center to view space
    const Core::Vec3 center = aabb.GetCenter();
    const Core::Vec3 camPos = camera.GetPosition();
    const Core::Vec3 toCenter = center - camPos;

    // Check if behind camera
    const Core::Vec3 camForward = (camera.GetTarget() - camPos).Normalized();
    const float viewZ = toCenter.Dot(camForward);
    if (viewZ <= 0.0f) return true; // Behind camera

    // Calculate normalized depth [0, 1]
    const float nearZ = camera.GetNearZ();
    const float farZ = camera.GetFarZ();
    if (viewZ < nearZ || viewZ > farZ) return false;

    const float ndcDepth = (viewZ - nearZ) / (farZ - nearZ);

    // Approximate screen pixel size
    const float radius = aabb.GetExtents().Length();
    const float screenRadius = (radius / viewZ) * (m_mips[0].height * 0.5f);

    // Pick mip level where pixel radius is roughly 1-2 texels
    const int mipLevel = std::clamp(static_cast<int>(std::floor(std::log2(std::max(1.0f, screenRadius)))),
                                    0, static_cast<int>(m_mips.size() - 1));

    const auto& mip = m_mips[mipLevel];
    // Map center to screen texel
    const Core::u32 texX = std::min(mip.width - 1, static_cast<Core::u32>(mip.width * 0.5f));
    const Core::u32 texY = std::min(mip.height - 1, static_cast<Core::u32>(mip.height * 0.5f));

    const float occluderDepth = mip.depths[texY * mip.width + texX];

    // If AABB closest depth is strictly further than the conservative occluder depth in tile -> Occluded!
    return (ndcDepth > occluderDepth + 0.005f);
}

} // namespace Engine::Renderer
