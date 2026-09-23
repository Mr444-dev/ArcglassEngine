#include "engine/world/SpatialBVH.h"
#include <algorithm>
#include <stack>

namespace Engine::World {

void SpatialBVH::Clear() {
    m_nodes.clear();
}

void SpatialBVH::Build(const std::vector<std::pair<EntityID, Core::AABB>>& objects) {
    Clear();
    if (objects.empty()) return;

    std::vector<std::pair<EntityID, Core::AABB>> workList = objects;
    m_nodes.reserve(objects.size() * 2);
    BuildRecursive(workList, 0, workList.size());
}

Core::u32 SpatialBVH::BuildRecursive(std::vector<std::pair<EntityID, Core::AABB>>& objects, size_t start, size_t end) {
    const size_t count = end - start;
    const Core::u32 nodeIdx = static_cast<Core::u32>(m_nodes.size());
    m_nodes.emplace_back();

    // 1. Compute total bounds
    Core::AABB totalBounds = objects[start].second;
    for (size_t i = start + 1; i < end; ++i) {
        totalBounds.Encapsulate(objects[i].second);
    }
    m_nodes[nodeIdx].bounds = totalBounds;

    // 2. Leaf node
    if (count <= 1) {
        m_nodes[nodeIdx].entity = objects[start].first;
        m_nodes[nodeIdx].isLeaf = true;
        return nodeIdx;
    }

    // 3. Find longest axis
    const Core::Vec3 extent = totalBounds.GetExtents();
    int splitAxis = 0; // 0 = X, 1 = Y, 2 = Z
    if (extent.y > extent.x && extent.y > extent.z) splitAxis = 1;
    else if (extent.z > extent.x && extent.z > extent.y) splitAxis = 2;

    // Sort along the split axis
    auto comp = [splitAxis](const std::pair<EntityID, Core::AABB>& a, const std::pair<EntityID, Core::AABB>& b) {
        const Core::Vec3 cA = a.second.GetCenter();
        const Core::Vec3 cB = b.second.GetCenter();
        if (splitAxis == 0) return cA.x < cB.x;
        if (splitAxis == 1) return cA.y < cB.y;
        return cA.z < cB.z;
    };

    std::sort(objects.begin() + start, objects.begin() + end, comp);

    const size_t mid = start + count / 2;
    const Core::u32 leftChild = BuildRecursive(objects, start, mid);
    const Core::u32 rightChild = BuildRecursive(objects, mid, end);

    m_nodes[nodeIdx].leftChild = leftChild;
    m_nodes[nodeIdx].rightChild = rightChild;
    m_nodes[nodeIdx].isLeaf = false;

    return nodeIdx;
}

void SpatialBVH::QueryFrustum(const Renderer::Frustum& frustum, std::vector<EntityID>& outVisibleEntities) const {
    if (m_nodes.empty()) return;

    std::vector<Core::u32> nodeStack;
    nodeStack.reserve(64);
    nodeStack.push_back(0); // Root

    while (!nodeStack.empty()) {
        const Core::u32 current = nodeStack.back();
        nodeStack.pop_back();

        const auto& node = m_nodes[current];

        // Frustum test against node bounding box
        if (!frustum.IntersectsAABB(node.bounds)) {
            continue; // Subtree completely outside frustum
        }

        if (node.isLeaf) {
            if (node.entity != NULL_ENTITY) {
                outVisibleEntities.push_back(node.entity);
            }
        } else {
            if (node.leftChild != 0xFFFFFFFF) nodeStack.push_back(node.leftChild);
            if (node.rightChild != 0xFFFFFFFF) nodeStack.push_back(node.rightChild);
        }
    }
}

} // namespace Engine::World
