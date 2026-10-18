#include "engine/world/TransformHierarchy.h"
#include "engine/core/Log.h"
#include <unordered_set>

namespace Engine::World {

bool TransformHierarchy::AttachChild(EntityID parent, EntityID child, Registry& registry) {
    if (parent == child || !registry.Has<TransformComponent>(parent) || !registry.Has<TransformComponent>(child)) {
        ENGINE_LOG_ERROR("TransformHierarchy: Refusing self-parenting or an entity without a transform.");
        return false;
    }

    const size_t entityLimit = registry.GetPool<TransformComponent>().GetEntities().size();
    EntityID ancestor = parent;
    size_t steps = 0;
    while (ancestor != NULL_ENTITY) {
        if (ancestor == child) {
            ENGINE_LOG_ERROR("TransformHierarchy: Refusing attachment that would create a parent cycle.");
            return false;
        }
        if (!registry.Has<TransformComponent>(ancestor) || ++steps > entityLimit) {
            ENGINE_LOG_ERROR("TransformHierarchy: Refusing attachment through an invalid or cyclic ancestor chain.");
            return false;
        }
        ancestor = registry.Get<TransformComponent>(ancestor).parent;
    }

    DetachChild(child, registry);

    auto& parentTransform = registry.Get<TransformComponent>(parent);
    auto& childTransform = registry.Get<TransformComponent>(child);
    childTransform.parent = parent;
    childTransform.nextSibling = parentTransform.firstChild;
    childTransform.prevSibling = NULL_ENTITY;

    if (parentTransform.firstChild != NULL_ENTITY && registry.Has<TransformComponent>(parentTransform.firstChild)) {
        registry.Get<TransformComponent>(parentTransform.firstChild).prevSibling = child;
    }
    parentTransform.firstChild = child;
    m_needsSort = true;
    return true;
}

void TransformHierarchy::DetachChild(EntityID child, Registry& registry) {
    if (!registry.Has<TransformComponent>(child)) return;
    auto& childTransform = registry.Get<TransformComponent>(child);
    if (childTransform.parent == NULL_ENTITY) return;

    const EntityID parent = childTransform.parent;
    if (registry.Has<TransformComponent>(parent)) {
        auto& parentTransform = registry.Get<TransformComponent>(parent);
        if (parentTransform.firstChild == child) parentTransform.firstChild = childTransform.nextSibling;
    }
    if (childTransform.prevSibling != NULL_ENTITY && registry.Has<TransformComponent>(childTransform.prevSibling)) {
        registry.Get<TransformComponent>(childTransform.prevSibling).nextSibling = childTransform.nextSibling;
    }
    if (childTransform.nextSibling != NULL_ENTITY && registry.Has<TransformComponent>(childTransform.nextSibling)) {
        registry.Get<TransformComponent>(childTransform.nextSibling).prevSibling = childTransform.prevSibling;
    }

    childTransform.parent = NULL_ENTITY;
    childTransform.nextSibling = NULL_ENTITY;
    childTransform.prevSibling = NULL_ENTITY;
    m_needsSort = true;
}

void TransformHierarchy::RebuildOrder(Registry& registry) {
    m_sortedEntities.clear();
    const auto& pool = registry.GetPool<TransformComponent>();
    const auto& entities = pool.GetEntities();
    m_sortedEntities.reserve(entities.size());

    std::unordered_set<EntityID> visited;
    visited.reserve(entities.size());
    std::vector<EntityID> stack;
    std::vector<EntityID> children;
    stack.reserve(entities.size());
    children.reserve(16);

    const auto traverse = [&](EntityID root) {
        stack.push_back(root);
        while (!stack.empty()) {
            const EntityID entity = stack.back();
            stack.pop_back();
            if (!registry.Has<TransformComponent>(entity) || !visited.insert(entity).second) continue;
            m_sortedEntities.push_back(entity);

            children.clear();
            std::unordered_set<EntityID> siblingChain;
            EntityID child = registry.Get<TransformComponent>(entity).firstChild;
            while (child != NULL_ENTITY && siblingChain.insert(child).second && children.size() < entities.size()) {
                if (!registry.Has<TransformComponent>(child)) break;
                const auto& childTransform = registry.Get<TransformComponent>(child);
                const EntityID nextSibling = childTransform.nextSibling;
                if (childTransform.parent == entity) children.push_back(child);
                child = nextSibling;
            }
            for (auto it = children.rbegin(); it != children.rend(); ++it) stack.push_back(*it);
        }
    };

    for (EntityID entity : entities) {
        const auto& transform = pool.Get(entity);
        if (transform.parent == NULL_ENTITY || !registry.Has<TransformComponent>(transform.parent)) {
            if (transform.parent != NULL_ENTITY) DetachChild(entity, registry);
            traverse(entity);
        }
    }

    // Recover from cycles or broken child links in loaded/edited scene data without recursion.
    for (EntityID entity : entities) {
        if (visited.contains(entity)) continue;
        ENGINE_LOG_ERROR("TransformHierarchy: Repairing an unreachable or cyclic entity {} as a root.", entity);
        DetachChild(entity, registry);
        auto& transform = registry.Get<TransformComponent>(entity);
        transform.parent = NULL_ENTITY;
        transform.prevSibling = NULL_ENTITY;
        transform.nextSibling = NULL_ENTITY;
        traverse(entity);
    }

    m_needsSort = false;
}

void TransformHierarchy::UpdateWorldTransforms(Registry& registry) {
    if (m_needsSort) RebuildOrder(registry);

    for (EntityID entity : m_sortedEntities) {
        auto& transform = registry.Get<TransformComponent>(entity);
        transform.UpdateLocalMatrix();
        if (transform.parent != NULL_ENTITY && registry.Has<TransformComponent>(transform.parent)) {
            transform.worldMatrix = transform.localMatrix * registry.Get<TransformComponent>(transform.parent).worldMatrix;
        } else {
            transform.worldMatrix = transform.localMatrix;
        }
    }
}

} // namespace Engine::World
