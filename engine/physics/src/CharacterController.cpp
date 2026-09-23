#include "engine/physics/CharacterController.h"
#include "engine/physics/PhysicsScene.h"
#include "engine/physics/Collision.h"

namespace Engine::Physics {

void CharacterController::Initialize(const Core::Vec3& initialPosition, float radius, float height) {
    m_position = initialPosition;
    m_radius = radius;
    m_height = height;
    m_velocity = {0.0f, 0.0f, 0.0f};
    const float feetY = m_position.y - m_height * 0.5f;
    m_isGrounded = (feetY <= 0.01f);
}

void CharacterController::Move(const Core::Vec3& displacement, float deltaTime, PhysicsScene& scene) {
    (void)scene;

    // 1. Horizontal displacement
    const Core::Vec3 horizontalMove{displacement.x, 0.0f, displacement.z};
    m_position = m_position + horizontalMove;

    // 2. Vertical displacement & Gravity
    const float halfH = m_height * 0.5f;
    m_position.y += (displacement.y + m_velocity.y * deltaTime);

    // 3. Ground contact determination
    if (m_position.y <= halfH + 1e-4f) {
        m_position.y = halfH;
        m_isGrounded = true;
        m_velocity.y = 0.0f;
    } else {
        m_isGrounded = false;
        m_velocity.y += -9.81f * deltaTime;
    }
}

} // namespace Engine::Physics
