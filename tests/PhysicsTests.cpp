#include "engine/core/BaseTypes.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "engine/world/Scene.h"
#include "engine/physics/PhysicsTypes.h"
#include "engine/physics/ColliderComponent.h"
#include "engine/physics/RigidBodyComponent.h"
#include "engine/physics/Collision.h"
#include "engine/physics/CharacterController.h"
#include "engine/physics/PhysicsScene.h"
#include <cmath>

void TestNarrowphaseCollisions() {
    using namespace Engine::Physics;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Narrowphase Collision Primitives ---");

    ContactPoint cp{};

    // 1. Sphere-Sphere
    bool hitSS = Collision::IntersectSphereSphere({0, 0, 0}, 1.0f, {1.5f, 0, 0}, 1.0f, cp);
    ENGINE_ASSERT(hitSS, "Overlapping spheres should collide");
    ENGINE_ASSERT(std::abs(cp.penetrationDepth - 0.5f) < 1e-4f, "Sphere-sphere penetration depth error");

    bool missSS = Collision::IntersectSphereSphere({0, 0, 0}, 1.0f, {3.0f, 0, 0}, 1.0f, cp);
    ENGINE_ASSERT(!missSS, "Distant spheres should not collide");

    // 2. Sphere-Box
    bool hitSB = Collision::IntersectSphereBox({0, 1.2f, 0}, 0.5f, {0, 0, 0}, {1, 1, 1}, cp);
    ENGINE_ASSERT(hitSB, "Sphere touching box top should collide");

    // 3. Box-Box SAT
    bool hitBB = Collision::IntersectBoxBox({0, 0, 0}, {1, 1, 1}, {1.8f, 0, 0}, {1, 1, 1}, cp);
    ENGINE_ASSERT(hitBB, "Overlapping boxes should collide");
    ENGINE_ASSERT(std::abs(cp.penetrationDepth - 0.2f) < 1e-4f, "Box-box penetration depth error");

    // 4. Raycast AABB
    Ray ray{{0, 5.0f, 0}, {0, -1.0f, 0}, 10.0f};
    RaycastHit rayHit{};
    AABB boxAABB{{-1, -1, -1}, {1, 1, 1}};
    bool hitRay = Collision::RaycastAABB(ray, boxAABB, rayHit);
    ENGINE_ASSERT(hitRay, "Raycast should hit AABB");
    ENGINE_ASSERT(std::abs(rayHit.distance - 4.0f) < 1e-4f, "Raycast distance mismatch (expected 4.0)");
    ENGINE_ASSERT(std::abs(rayHit.normal.y - 1.0f) < 1e-4f, "Raycast normal should point up");

    ENGINE_LOG_INFO("Narrowphase collision primitives passed all checks.");
}

void TestRigidBodyGravityAndGroundCollision() {
    using namespace Engine::World;
    using namespace Engine::Physics;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing RigidBody Gravity Fall & Ground Plane Collision ---");

    Scene scene;
    PhysicsScene physicsScene;

    // Create Static Ground Box at Y = -0.5, half extents = {50, 0.5, 50} (top surface at Y = 0)
    Entity ground = scene.CreateEntity("Ground");
    ground.GetComponent<TransformComponent>().position = {0.0f, -0.5f, 0.0f};
    auto& groundCol = ground.AddComponent<ColliderComponent>();
    groundCol.type = ColliderType::Box;
    groundCol.boxHalfExtents = {50.0f, 0.5f, 50.0f};
    auto& groundRb = ground.AddComponent<RigidBodyComponent>();
    groundRb.motionType = MotionType::Static;

    // Create Dynamic Falling Cube at Y = 5.0, half extents = {0.5, 0.5, 0.5} (bottom face at Y = 4.5)
    Entity fallingBox = scene.CreateEntity("FallingBox");
    fallingBox.GetComponent<TransformComponent>().position = {0.0f, 5.0f, 0.0f};
    auto& boxCol = fallingBox.AddComponent<ColliderComponent>();
    boxCol.type = ColliderType::Box;
    boxCol.boxHalfExtents = {0.5f, 0.5f, 0.5f};
    auto& boxRb = fallingBox.AddComponent<RigidBodyComponent>();
    boxRb.motionType = MotionType::Dynamic;
    boxRb.SetMass(2.0f);

    // Simulate 1.0 second (60 frames at 60 Hz)
    for (int frame = 0; frame < 60; ++frame) {
        physicsScene.Update(0.016667f, scene);
    }

    const float finalY = fallingBox.GetComponent<TransformComponent>().position.y;
    ENGINE_LOG_INFO("Falling box final position Y: {:.2f} (Expected resting on ground: ~0.50)", finalY);

    // Box has half-height 0.5, ground surface is at Y = 0.0, so resting center Y is approx 0.5
    ENGINE_ASSERT(finalY >= 0.40f && finalY <= 0.65f, "Rigid body failed to rest on ground plane");
    ENGINE_LOG_INFO("RigidBody gravity and ground collision verified.");
}

void TestKinematicCharacterController() {
    using namespace Engine::Physics;
    using namespace Engine::World;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Milestone Phase 5: Kinematic Character Controller ---");

    Scene scene;
    PhysicsScene physicsScene;

    CharacterController player;
    player.Initialize({0.0f, 1.0f, 0.0f}, 0.4f, 2.0f);

    ENGINE_ASSERT(player.GetPosition().y == 1.0f, "Character initial position mismatch");

    // Move player horizontally 5 units forward
    player.Move({5.0f, 0.0f, 0.0f}, 0.016f, physicsScene);
    ENGINE_ASSERT(std::abs(player.GetPosition().x - 5.0f) < 1e-4f, "Character displacement X mismatch");
    ENGINE_ASSERT(player.IsGrounded(), "Character should remain grounded on floor");

    // Simulate jump (upward displacement)
    player.Move({0.0f, 2.0f, 0.0f}, 0.016f, physicsScene);
    ENGINE_ASSERT(player.GetPosition().y > 1.0f, "Character failed to jump");

    // Gravity pull over subsequent frames (1.0 second to land from 2m jump)
    for (int i = 0; i < 60; ++i) {
        player.Move({0.0f, 0.0f, 0.0f}, 0.016f, physicsScene);
    }

    ENGINE_ASSERT(player.IsGrounded(), "Character should land back on ground");
    ENGINE_LOG_INFO("Kinematic Character Controller verified at final pos ({:.2f}, {:.2f}, {:.2f}).",
        player.GetPosition().x, player.GetPosition().y, player.GetPosition().z);
}

void RunPhysicsUnitTests() {
    TestNarrowphaseCollisions();
    TestRigidBodyGravityAndGroundCollision();
    TestKinematicCharacterController();
}
