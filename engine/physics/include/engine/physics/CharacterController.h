#pragma once

#include "engine/physics/PhysicsTypes.h"
#include "engine/core/MathTypes.h"

namespace Engine::Physics {

class PhysicsScene;

class CharacterController {
public:
    CharacterController() = default;
    ~CharacterController() = default;

    void Initialize(const Core::Vec3& initialPosition, float radius = 0.4f, float height = 1.8f);

    void Move(const Core::Vec3& displacement, float deltaTime, PhysicsScene& scene);

    [[nodiscard]] const Core::Vec3& GetPosition() const noexcept { return m_position; }
    void SetPosition(const Core::Vec3& pos) noexcept { m_position = pos; }

    [[nodiscard]] const Core::Vec3& GetVelocity() const noexcept { return m_velocity; }
    void SetVelocity(const Core::Vec3& vel) noexcept { m_velocity = vel; }

    [[nodiscard]] bool IsGrounded() const noexcept { return m_isGrounded; }
    [[nodiscard]] float GetRadius() const noexcept { return m_radius; }
    [[nodiscard]] float GetHeight() const noexcept { return m_height; }

    void SetStepHeight(float stepHeight) noexcept { m_stepHeight = stepHeight; }
    [[nodiscard]] float GetStepHeight() const noexcept { return m_stepHeight; }

private:
    Core::Vec3 m_position{0.0f, 0.0f, 0.0f};
    Core::Vec3 m_velocity{0.0f, 0.0f, 0.0f};
    float m_radius{0.4f};
    float m_height{1.8f};
    float m_stepHeight{0.3f};
    bool m_isGrounded{false};
};

} // namespace Engine::Physics
