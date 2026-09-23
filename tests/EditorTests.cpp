#include "engine/core/BaseTypes.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "editor/EditorApp.h"
#include "editor/GizmoController.h"

void TestSceneHierarchyOperations() {
    using namespace Engine::Editor;
    using namespace Engine::World;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing SceneHierarchyPanel Entity Operations ---");

    Scene scene;
    EditorContext context;
    context.scene = &scene;

    SceneHierarchyPanel hierarchy;

    // 1. Create entities
    Entity cube = hierarchy.CreateCubeEntity(context, "TestCube");
    ENGINE_ASSERT(context.selectedEntity == cube.GetID(), "Newly created cube should be selected");
    ENGINE_ASSERT(cube.HasComponent<MeshComponent>(), "Cube missing MeshComponent");

    Entity light = hierarchy.CreateLightEntity(context, "TestLight");
    ENGINE_ASSERT(context.selectedEntity == light.GetID(), "Newly created light should be selected");
    ENGINE_ASSERT(light.HasComponent<LightComponent>(), "Light missing LightComponent");

    // 2. Reparenting
    hierarchy.ReparentEntity(context, light.GetID(), cube.GetID());
    const auto& lightTransform = light.GetComponent<TransformComponent>();
    ENGINE_ASSERT(lightTransform.parent == cube.GetID(), "Reparenting failed");

    // 3. Render hierarchy tree
    hierarchy.OnRender(context);
    ENGINE_ASSERT(!hierarchy.GetRenderLogs().empty(), "Hierarchy render output should not be empty");

    // 4. Delete selected entity
    hierarchy.DeleteSelectedEntity(context);
    ENGINE_ASSERT(!context.HasSelection(), "Selection should be cleared on deletion");
    ENGINE_ASSERT(scene.GetEntityCount() == 1, "Entity count should be 1 after deletion");

    ENGINE_LOG_INFO("SceneHierarchyPanel entity operations passed all checks.");
}

void TestPropertiesPanelInspection() {
    using namespace Engine::Editor;
    using namespace Engine::World;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing PropertiesPanel Component Inspection ---");

    Scene scene;
    EditorContext context;
    context.scene = &scene;

    SceneHierarchyPanel hierarchy;
    PropertiesPanel properties;

    Entity cube = hierarchy.CreateCubeEntity(context, "PlayerCube");
    context.SelectEntity(cube.GetID());

    properties.OnRender(context);
    const auto& logs = properties.GetInspectorLogs();
    ENGINE_ASSERT(logs.size() >= 4, "Expected inspection logs for Tag, Transform, Mesh, Collider, RigidBody");

    // Modify transform
    auto& t = cube.GetComponent<TransformComponent>();
    t.position = {12.0f, 4.0f, -8.0f};

    properties.OnRender(context);
    ENGINE_LOG_INFO("PropertiesPanel inspected components for entity '{}'.", cube.GetID());
}

void TestGizmoManipulators() {
    using namespace Engine::Editor;
    using namespace Engine::World;
    using namespace Engine::Core;
    using namespace Engine::Renderer;

    Logger::LogMessage(LogLevel::Info, "--- Testing GizmoController 3D Manipulations ---");

    Scene scene;
    EditorContext context;
    context.scene = &scene;

    Entity entity = scene.CreateEntity("GizmoTarget");
    context.SelectEntity(entity.GetID());

    Camera dummyCam;

    // Apply translation
    GizmoController::ApplyTranslation(context, {3.0f, 0.0f, 0.0f});
    auto& t = entity.GetComponent<TransformComponent>();
    ENGINE_ASSERT(std::abs(t.position.x - 3.0f) < 1e-4f, "Gizmo translation X mismatch");

    // Mouse delta projection
    Vec3 delta = GizmoController::CalculateTranslationDelta(GizmoAxis::Y, 0.0f, -50.0f, dummyCam);
    GizmoController::ApplyTranslation(context, delta);
    ENGINE_ASSERT(t.position.y > 0.5f, "Gizmo axis projection failed");

    ENGINE_LOG_INFO("GizmoController 3D transformations verified.");
}

void TestEditorAppFullLifecycle() {
    using namespace Engine::Editor;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Milestone Phase 6: Full EditorApp Lifecycle & Simulation ---");

    EditorApp editor;
    editor.Initialize();

    auto& context = editor.GetContext();
    ENGINE_ASSERT(context.scene != nullptr, "Editor active scene is null");
    ENGINE_ASSERT(context.scene->GetEntityCount() == 3, "Expected 3 default entities in starter scene");

    // Start simulation (Play Mode)
    editor.GetViewportPanel().OnPlayClicked(context);
    ENGINE_ASSERT(context.isPlaying, "Play mode failed to activate");

    // Run 10 frames
    for (int frame = 0; frame < 10; ++frame) {
        editor.Update(0.016667f);
    }

    // Stop simulation
    editor.GetViewportPanel().OnStopClicked(context);
    ENGINE_ASSERT(!context.isPlaying, "Play mode failed to stop");

    ENGINE_LOG_INFO("Full EditorApp lifecycle and play mode simulation verified.");
}

void RunEditorUnitTests() {
    TestSceneHierarchyOperations();
    TestPropertiesPanelInspection();
    TestGizmoManipulators();
    TestEditorAppFullLifecycle();
}
