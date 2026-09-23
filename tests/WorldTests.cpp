#include "engine/core/BaseTypes.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "engine/world/Scene.h"
#include <chrono>

void TestEntityAndComponentLifecycle() {
    using namespace Engine::World;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Entity & Component Lifecycle ---");

    Scene scene;
    Entity e1 = scene.CreateEntity("TestEntity1");
    ENGINE_ASSERT(e1.IsValid(), "Failed to create entity");
    ENGINE_ASSERT(e1.HasComponent<TagComponent>(), "Entity missing default TagComponent");
    ENGINE_ASSERT(e1.HasComponent<TransformComponent>(), "Entity missing default TransformComponent");

    // Add MeshComponent
    MeshComponent& mesh = e1.AddComponent<MeshComponent>();
    mesh.meshIndex = 42;
    mesh.materialIndex = 7;
    ENGINE_ASSERT(e1.HasComponent<MeshComponent>(), "MeshComponent emplace failed");
    ENGINE_ASSERT(e1.GetComponent<MeshComponent>().meshIndex == 42, "MeshComponent property mismatch");

    // Remove MeshComponent
    e1.RemoveComponent<MeshComponent>();
    ENGINE_ASSERT(!e1.HasComponent<MeshComponent>(), "MeshComponent removal failed");

    // Destroy Entity
    scene.DestroyEntity(e1.GetID());
    ENGINE_ASSERT(scene.GetEntityCount() == 0, "Entity destruction failed");

    ENGINE_LOG_INFO("Entity and Component lifecycle passed all checks.");
}

void TestTransformHierarchyPropagation() {
    using namespace Engine::World;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Transform Hierarchy Propagation ---");

    Scene scene;

    Entity parent = scene.CreateEntity("Parent");
    Entity child = scene.CreateEntity("Child");
    Entity grandchild = scene.CreateEntity("Grandchild");

    auto& pT = parent.GetComponent<TransformComponent>();
    pT.position = {10.0f, 0.0f, 0.0f};

    auto& cT = child.GetComponent<TransformComponent>();
    cT.position = {5.0f, 0.0f, 0.0f};

    auto& gT = grandchild.GetComponent<TransformComponent>();
    gT.position = {2.0f, 0.0f, 0.0f};

    // Build hierarchy: Parent -> Child -> Grandchild
    scene.GetHierarchy().AttachChild(parent.GetID(), child.GetID(), scene.GetRegistry());
    scene.GetHierarchy().AttachChild(child.GetID(), grandchild.GetID(), scene.GetRegistry());

    // Update scene
    scene.Update(0.016f);

    // Parent world X should be 10
    // Child world X should be 10 + 5 = 15
    // Grandchild world X should be 15 + 2 = 17
    const float childWorldX = child.GetComponent<TransformComponent>().worldMatrix.m[12];
    const float grandWorldX = grandchild.GetComponent<TransformComponent>().worldMatrix.m[12];

    ENGINE_ASSERT(std::abs(childWorldX - 15.0f) < 1e-4f, "Child world position incorrect");
    ENGINE_ASSERT(std::abs(grandWorldX - 17.0f) < 1e-4f, "Grandchild world position incorrect");

    ENGINE_LOG_INFO("Transform hierarchy propagation verified (Parent: 10, Child: {}, Grandchild: {}).",
        childWorldX, grandWorldX);
}

void Test50000EntitiesStressTest() {
    using namespace Engine::World;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Milestone Phase 3: 50,000 Entities Hierarchy Stress Test ---");

    Scene scene;
    constexpr size_t ENTITY_COUNT = 50000;

    auto tStartAlloc = std::chrono::high_resolution_clock::now();

    Entity root = scene.CreateEntity("RootEntity");
    EntityID currentParent = root.GetID();

    // Create 50,000 entities linked in a deep/branching hierarchy
    for (size_t i = 1; i < ENTITY_COUNT; ++i) {
        Entity e = scene.CreateEntity("Child_" + std::to_string(i));
        e.GetComponent<TransformComponent>().position = {0.1f, 0.0f, 0.0f};

        scene.GetHierarchy().AttachChild(currentParent, e.GetID(), scene.GetRegistry());

        // Branch every 10 levels
        if (i % 10 == 0) {
            currentParent = e.GetID();
        }
    }

    auto tEndAlloc = std::chrono::high_resolution_clock::now();
    double allocTimeMs = std::chrono::duration<double, std::milli>(tEndAlloc - tStartAlloc).count();
    ENGINE_LOG_INFO("Allocated and linked {} entities in hierarchy in {:.2f} ms.", ENTITY_COUNT, allocTimeMs);

    // Benchmark linear update
    auto tStartUpdate = std::chrono::high_resolution_clock::now();

    scene.Update(0.016f);

    auto tEndUpdate = std::chrono::high_resolution_clock::now();
    double updateTimeMs = std::chrono::duration<double, std::milli>(tEndUpdate - tStartUpdate).count();

    const double fpsEquivalent = (updateTimeMs > 0.0) ? (1000.0 / updateTimeMs) : 9999.0;
    ENGINE_LOG_INFO("Updated 50,000 entities in {:.2f} ms (Equivalent single-thread rate: {:.0f} FPS).", 
        updateTimeMs, fpsEquivalent);

    ENGINE_ASSERT(scene.GetEntityCount() == ENTITY_COUNT, "Entity count mismatch");
    ENGINE_LOG_INFO("50,000 Entities stress test successfully completed.");
}

void TestSpatialBVHFrustumCulling() {
    using namespace Engine::World;
    using namespace Engine::Core;
    using namespace Engine::Renderer;

    Logger::LogMessage(LogLevel::Info, "--- Testing Spatial BVH Frustum Culling ---");

    Scene scene;

    // Create 100 entities with MeshComponent spread along Z
    for (int i = 0; i < 100; ++i) {
        Entity e = scene.CreateEntity("Mesh_" + std::to_string(i));
        auto& t = e.GetComponent<TransformComponent>();
        t.position = {0.0f, 0.0f, static_cast<float>(i * 5 - 250)}; // from -250 to +245

        auto& mesh = e.AddComponent<MeshComponent>();
        mesh.localAABB = {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
        mesh.isVisible = true;
    }

    scene.Update(0.016f);

    Camera camera;
    camera.SetPosition({0.0f, 0.0f, 0.0f});
    camera.SetTarget({0.0f, 0.0f, 100.0f});
    camera.SetLens(0.785398f, 16.0f / 9.0f, 0.1f, 100.0f);
    camera.UpdateMatrices();

    std::vector<EntityID> visibleEntities;
    scene.GetBVH().QueryFrustum(camera.GetFrustum(), visibleEntities);

    ENGINE_LOG_INFO("BVH Frustum Query: {} of 100 entities visible (entities behind camera culled).", visibleEntities.size());
    ENGINE_ASSERT(!visibleEntities.empty(), "Expected visible entities inside frustum");
    ENGINE_ASSERT(visibleEntities.size() < 100, "Frustum should cull entities behind camera");

    ENGINE_LOG_INFO("Spatial BVH Frustum culling verified.");
}

void RunWorldUnitTests() {
    TestEntityAndComponentLifecycle();
    TestTransformHierarchyPropagation();
    Test50000EntitiesStressTest();
    TestSpatialBVHFrustumCulling();
}
