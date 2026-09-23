#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/MathTypes.h"
#include "engine/world/Entity.h"
#include "engine/renderer/Camera.h"
#include <vector>

namespace Engine::World {

struct BVHNode {
    Core::AABB bounds;
    Core::u32 leftChild{0xFFFFFFFF};
    Core::u32 rightChild{0xFFFFFFFF};
    EntityID entity{NULL_ENTITY};
    bool isLeaf{false};
};

class SpatialBVH {
public:
    SpatialBVH() = default;
    ~SpatialBVH() = default;

    void Build(const std::vector<std::pair<EntityID, Core::AABB>>& objects);
    void Clear();

    void QueryFrustum(const Renderer::Frustum& frustum, std::vector<EntityID>& outVisibleEntities) const;

    [[nodiscard]] size_t GetNodeCount() const noexcept { return m_nodes.size(); }
    [[nodiscard]] const Core::AABB& GetRootBounds() const noexcept {
        static Core::AABB emptyAABB;
        return m_nodes.empty() ? emptyAABB : m_nodes[0].bounds;
    }

private:
    Core::u32 BuildRecursive(std::vector<std::pair<EntityID, Core::AABB>>& objects, size_t start, size_t end);

    std::vector<BVHNode> m_nodes;
};

} // namespace Engine::World
