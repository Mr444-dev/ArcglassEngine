#pragma once

#include "engine/core/BaseTypes.h"
#include <limits>

namespace Engine::World {

using EntityID = Core::u32;
inline constexpr EntityID NULL_ENTITY = std::numeric_limits<EntityID>::max();

class Scene;

class Entity {
public:
    Entity() = default;
    Entity(EntityID id, Scene* scene) : m_id(id), m_scene(scene) {}

    [[nodiscard]] EntityID GetID() const noexcept { return m_id; }
    [[nodiscard]] bool IsValid() const noexcept { return m_id != NULL_ENTITY && m_scene != nullptr; }

    template <typename T, typename... Args>
    T& AddComponent(Args&&... args);

    template <typename T>
    [[nodiscard]] T& GetComponent();

    template <typename T>
    [[nodiscard]] const T& GetComponent() const;

    template <typename T>
    [[nodiscard]] bool HasComponent() const noexcept;

    template <typename T>
    void RemoveComponent();

    [[nodiscard]] Scene* GetScene() const noexcept { return m_scene; }

    bool operator==(const Entity& o) const noexcept { return m_id == o.m_id && m_scene == o.m_scene; }
    bool operator!=(const Entity& o) const noexcept { return !(*this == o); }

private:
    EntityID m_id{NULL_ENTITY};
    Scene* m_scene{nullptr};
};

} // namespace Engine::World
