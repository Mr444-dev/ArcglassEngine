#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/world/Entity.h"
#include "engine/world/Components.h"
#include "engine/world/Registry.h"
#include <vector>

namespace Engine::World {

class TransformHierarchy {
public:
    TransformHierarchy() = default;
    ~TransformHierarchy() = default;

    void AttachChild(EntityID parent, EntityID child, Registry& registry);
    void DetachChild(EntityID child, Registry& registry);

    void UpdateWorldTransforms(Registry& registry);
    void MarkDirty() noexcept { m_needsSort = true; }

    [[nodiscard]] size_t GetEntityCount() const noexcept { return m_sortedEntities.size(); }
    [[nodiscard]] const std::vector<EntityID>& GetSortedEntities() const noexcept { return m_sortedEntities; }

private:
    void RebuildOrder(Registry& registry);
    void BuildDFS(EntityID entity, Registry& registry);

    std::vector<EntityID> m_sortedEntities;
    bool m_needsSort{true};
};

} // namespace Engine::World
