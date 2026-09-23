#include "engine/world/TransformHierarchy.h"
#include "engine/core/Assert.h"

namespace Engine::World {

void TransformHierarchy::AttachChild(EntityID parent, EntityID child, Registry& registry) {
    ENGINE_ASSERT(parent != child, "Cannot attach entity as child of itself");
    ENGINE_ASSERT(registry.Has<TransformComponent>(parent), "Parent must have TransformComponent");
    ENGINE_ASSERT(registry.Has<TransformComponent>(child), "Child must have TransformComponent");

    DetachChild(child, registry);

    auto& pTransform = registry.Get<TransformComponent>(parent);
    auto& cTransform = registry.Get<TransformComponent>(child);

    cTransform.parent = parent;
    cTransform.nextSibling = pTransform.firstChild;
    cTransform.prevSibling = NULL_ENTITY;

    if (pTransform.firstChild != NULL_ENTITY) {
        auto& firstChildTransform = registry.Get<TransformComponent>(pTransform.firstChild);
        firstChildTransform.prevSibling = child;
    }
    pTransform.firstChild = child;

    m_needsSort = true;
}

void TransformHierarchy::DetachChild(EntityID child, Registry& registry) {
    if (!registry.Has<TransformComponent>(child)) return;

    auto& cTransform = registry.Get<TransformComponent>(child);
    if (cTransform.parent == NULL_ENTITY) return;

    auto& pTransform = registry.Get<TransformComponent>(cTransform.parent);

    // If first child
    if (pTransform.firstChild == child) {
        pTransform.firstChild = cTransform.nextSibling;
    }

    // Unlink siblings
    if (cTransform.prevSibling != NULL_ENTITY) {
        auto& prev = registry.Get<TransformComponent>(cTransform.prevSibling);
        prev.nextSibling = cTransform.nextSibling;
    }

    if (cTransform.nextSibling != NULL_ENTITY) {
        auto& next = registry.Get<TransformComponent>(cTransform.nextSibling);
        next.prevSibling = cTransform.prevSibling;
    }

    cTransform.parent = NULL_ENTITY;
    cTransform.nextSibling = NULL_ENTITY;
    cTransform.prevSibling = NULL_ENTITY;

    m_needsSort = true;
}

void TransformHierarchy::BuildDFS(EntityID entity, Registry& registry) {
    m_sortedEntities.push_back(entity);

    const auto& t = registry.Get<TransformComponent>(entity);
    EntityID child = t.firstChild;
    while (child != NULL_ENTITY) {
        BuildDFS(child, registry);
        const auto& childTransform = registry.Get<TransformComponent>(child);
        child = childTransform.nextSibling;
    }
}

void TransformHierarchy::RebuildOrder(Registry& registry) {
    m_sortedEntities.clear();
    const auto& pool = registry.GetPool<TransformComponent>();
    const auto& entities = pool.GetEntities();

    m_sortedEntities.reserve(entities.size());

    // Traverse all root entities (parent == NULL_ENTITY)
    for (EntityID e : entities) {
        const auto& t = pool.Get(e);
        if (t.parent == NULL_ENTITY) {
            BuildDFS(e, registry);
        }
    }

    m_needsSort = false;
}

void TransformHierarchy::UpdateWorldTransforms(Registry& registry) {
    if (m_needsSort) {
        RebuildOrder(registry);
    }

    // Linear non-recursive cache-friendly loop
    for (EntityID e : m_sortedEntities) {
        auto& t = registry.Get<TransformComponent>(e);
        t.UpdateLocalMatrix();

        if (t.parent != NULL_ENTITY) {
            const auto& parentT = registry.Get<TransformComponent>(t.parent);
            t.worldMatrix = t.localMatrix * parentT.worldMatrix;
        } else {
            t.worldMatrix = t.localMatrix;
        }
    }
}

} // namespace Engine::World
