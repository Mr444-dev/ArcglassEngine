#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/world/Entity.h"
#include "engine/world/Components.h"
#include "engine/world/Registry.h"
#include "engine/world/TransformHierarchy.h"
#include "engine/world/SpatialBVH.h"
#include <string>
#include <vector>

namespace Engine::World {

class Scene {
public:
    Scene();
    ~Scene();

    Entity CreateEntity(const std::string& name = "Entity");
    void DestroyEntity(EntityID entity);

    void Update(float deltaTime);

    [[nodiscard]] Registry& GetRegistry() noexcept { return m_registry; }
    [[nodiscard]] const Registry& GetRegistry() const noexcept { return m_registry; }

    [[nodiscard]] TransformHierarchy& GetHierarchy() noexcept { return m_hierarchy; }
    [[nodiscard]] const TransformHierarchy& GetHierarchy() const noexcept { return m_hierarchy; }

    [[nodiscard]] const SpatialBVH& GetBVH() const noexcept { return m_bvh; }

    [[nodiscard]] size_t GetEntityCount() const noexcept { return m_entities.size(); }

private:
    Registry m_registry;
    TransformHierarchy m_hierarchy;
    SpatialBVH m_bvh;

    std::vector<EntityID> m_entities;
    EntityID m_nextEntityID{0};
};

// Template implementations for Entity
template <typename T, typename... Args>
T& Entity::AddComponent(Args&&... args) {
    ENGINE_ASSERT(IsValid(), "Invalid Entity handle");
    return m_scene->GetRegistry().Emplace<T>(m_id, std::forward<Args>(args)...);
}

template <typename T>
T& Entity::GetComponent() {
    ENGINE_ASSERT(IsValid(), "Invalid Entity handle");
    return m_scene->GetRegistry().Get<T>(m_id);
}

template <typename T>
const T& Entity::GetComponent() const {
    ENGINE_ASSERT(IsValid(), "Invalid Entity handle");
    return m_scene->GetRegistry().Get<T>(m_id);
}

template <typename T>
bool Entity::HasComponent() const noexcept {
    if (!IsValid()) return false;
    return m_scene->GetRegistry().Has<T>(m_id);
}

template <typename T>
void Entity::RemoveComponent() {
    ENGINE_ASSERT(IsValid(), "Invalid Entity handle");
    m_scene->GetRegistry().Remove<T>(m_id);
}

} // namespace Engine::World
