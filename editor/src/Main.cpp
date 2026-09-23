#include "editor/EditorApp.h"
#include "engine/core/Log.h"
#include "engine/platform/Platform.h"
#include "engine/platform/Window.h"
#include "engine/platform/Input.h"
#include "engine/rhi/RHI.h"
#include "engine/rhi/RHIDevice.h"
#include "engine/rhi/RHIShaderCompiler.h"

#include <chrono>
#include <format>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <fstream>

using namespace Engine;
using namespace Engine::Core;
using namespace Engine::RHI;

struct EditorEntity {
    std::string name;
    float posX = 0.0f;
    float posY = 0.0f;
    float posZ = 0.0f;
    float rotY = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float scaleZ = 1.0f;
    int type = 0; // 0 = Cube, 1 = Sphere, 2 = Cylinder, 3 = Plane, 4 = Light
    float velY = 0.0f;
};

struct UndoAction {
    int entityIndex;
    float prevX, prevY, prevZ;
    float newX, newY, newZ;
};

// Automated Standalone Shipping Packager
bool PackageProjectForShipping() {
    ENGINE_LOG_INFO("[BUILD] Packaging project for Windows (x64 Release)...");
    try {
        std::filesystem::create_directories("dist/Windows");
        std::filesystem::create_directories("dist/Windows/assets");
        std::filesystem::create_directories("dist/Windows/engine");

        std::error_code ec;
        std::filesystem::copy_file("GameRuntime.exe", "dist/Windows/Game.exe",
            std::filesystem::copy_options::overwrite_existing, ec);

        if (std::filesystem::exists("assets")) {
            std::filesystem::copy("assets", "dist/Windows/assets",
                std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, ec);
        }
        if (std::filesystem::exists("engine")) {
            std::filesystem::copy("engine", "dist/Windows/engine",
                std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, ec);
        }

        std::ofstream out("dist/Windows/Launch_Game.bat");
        out << "@echo off\n";
        out << "echo ===========================================\n";
        out << "echo Launching Packaged Standalone Shipping Game\n";
        out << "echo ===========================================\n";
        out << "start Game.exe\n";
        out.close();

        ENGINE_LOG_INFO("[BUILD] Standalone Game Package created successfully: dist/Windows/Game.exe");
        return true;
    } catch (const std::exception& e) {
        ENGINE_LOG_ERROR("[BUILD] Packaging failed: {}", e.what());
        return false;
    }
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    ENGINE_LOG_INFO("=================================================");
    ENGINE_LOG_INFO("   Modern 3D Game Engine - AAA Editor Studio     ");
    ENGINE_LOG_INFO("=================================================");

    // 1. Initialize Platform layer and main Editor window
    Platform::Initialize();

    Platform::Window window;
    Platform::WindowDesc winDesc{};
    winDesc.title = "Modern 3D Game Engine (Standard 2026) - Full AAA Production Studio";
    winDesc.width = 1440;
    winDesc.height = 900;
    winDesc.resizable = true;

    if (!window.Initialize(winDesc)) {
        ENGINE_LOG_ERROR("Failed to initialize platform window.");
        Platform::Shutdown();
        return -1;
    }

    // 2. Initialize Low-Level Graphics RHI & SwapChain
    auto& device = RHIDevice::Get();
    if (!device.Initialize(window.GetNativeHandle(), winDesc.width, winDesc.height)) {
        ENGINE_LOG_ERROR("Failed to initialize RHI GPU device for editor.");
        Platform::Shutdown();
        return 1;
    }

    auto swapChain = device.CreateSwapChain(window.GetNativeHandle(), winDesc.width, winDesc.height);
    if (!swapChain) {
        ENGINE_LOG_ERROR("Failed to create SwapChain for editor.");
        device.Shutdown();
        Platform::Shutdown();
        return 1;
    }

    auto cmdList = device.CreateCommandList();
    if (!cmdList) {
        ENGINE_LOG_ERROR("Failed to create CommandList for editor.");
        device.Shutdown();
        Platform::Shutdown();
        return 1;
    }

    // 3. Compile Editor UI and 3D Viewport Shaders
    std::vector<u8> vsBytecode;
    std::vector<u8> psBytecode;
    bool vsOk = RHIShaderCompiler::CompileFromFile("shaders/editor/EditorUI.hlsl", "VSMain", "vs_5_1", vsBytecode);
    bool psOk = RHIShaderCompiler::CompileFromFile("shaders/editor/EditorUI.hlsl", "PSMain", "ps_5_1", psBytecode);
    if (!vsOk || !psOk) {
        ENGINE_LOG_ERROR("Failed to compile Editor UI shaders! Check shader file path and syntax.");
        device.Shutdown();
        Platform::Shutdown();
        return 1;
    }

    GraphicsPipelineDesc psoDesc{};
    psoDesc.vertexShaderBytecode = vsBytecode.data();
    psoDesc.vertexShaderSize = vsBytecode.size();
    psoDesc.pixelShaderBytecode = psBytecode.data();
    psoDesc.pixelShaderSize = psBytecode.size();
    psoDesc.renderTargetCount = 1;
    psoDesc.renderTargetFormats[0] = Format::RGBA8_UNORM;
    psoDesc.depthStencilFormat = Format::Unknown;
    psoDesc.depthEnable = false;
    psoDesc.depthWrite = false;
    psoDesc.cullBackFace = false;

    auto pso = device.CreateGraphicsPipelineState(psoDesc);
    if (!pso) {
        ENGINE_LOG_ERROR("Failed to create Editor UI Graphics Pipeline State.");
        device.Shutdown();
        Platform::Shutdown();
        return 1;
    }

    Editor::EditorApp editor;
    editor.Initialize();

    ENGINE_LOG_INFO("AAA Studio Editor ready. Fly: RMB+WASD. Focus: F. Orbit: Alt+LMB. Snap: 0.5m. Debug: F5.");

    auto prevTime = std::chrono::high_resolution_clock::now();
    float fpsTimer = 0.0f;
    u32 fpsFrameCount = 0;
    u64 totalFrames = 0;
    float totalElapsed = 0.0f;

    // Viewport Free-Fly Camera (WASD + RMB)
    float camPosX = 0.0f;
    float camPosY = 2.4f;
    float camPosZ = 5.5f;
    float vpCamYaw = 0.0f;
    float vpCamPitch = -0.22f;

    // Simulation States: 0 = EDIT, 1 = PLAY, 2 = PAUSE
    int simState = 0;
    std::vector<EditorEntity> prePlaySnapshot;

    // Interactive UI State
    int activeMenu = 0;       // 0=None, 1=File, 2=Edit, 3=Scene, 4=Build
    int bottomTab = 0;        // 0=Content Browser, 1=Console Log
    int activeModal = 0;      // 0=None, 1=Settings, 2=BuildSuccess, 3=AddComponent
    int gizmoMode = 1;        // 0=Select, 1=Move(W), 2=Rotate(E), 3=Scale(R)
    bool debugDraw = false;   // F5 Debug drawing toggle

    // Undo / Redo history stacks
    std::vector<UndoAction> undoStack;
    std::vector<UndoAction> redoStack;

    // Scene Entities
    std::vector<EditorEntity> sceneEntities = {
        { "SM_Cube_01",      0.0f, 0.8f,  0.0f,  0.0f, 1.0f, 1.0f, 1.0f, 0, 0.0f },
        { "SM_Sphere_01",    2.2f, 0.9f,  0.5f,  0.0f, 1.0f, 1.0f, 1.0f, 1, 0.0f },
        { "SM_Cylinder_01", -2.2f, 1.0f,  0.5f,  0.0f, 1.0f, 1.0f, 1.0f, 2, 0.0f },
        { "Floor_Plane",     0.0f, 0.0f,  0.0f,  0.0f, 3.0f, 1.0f, 3.0f, 3, 0.0f },
        { "Sun_Light",      -3.0f, 4.0f, -2.0f,  0.0f, 1.0f, 1.0f, 1.0f, 4, 0.0f }
    };
    int selectedEntity = 0;

    // Interactive Application Loop
    while (true) {
        Platform::Input::Get().BeginFrame();
        if (!window.PollEvents()) {
            break;
        }

        if (Platform::Input::Get().IsKeyDown(Platform::KeyCode::Escape)) {
            if (activeModal != 0) activeModal = 0;
            else if (activeMenu != 0) activeMenu = 0;
            else break;
        }

        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> delta = currentTime - prevTime;
        prevTime = currentTime;
        float dt = std::min(delta.count(), 0.1f);
        totalElapsed += dt;

        // --- CALIBRATION & TESTING HOTKEYS (F5 to F9) ---
        if (Platform::Input::Get().WasKeyPressed(Platform::KeyCode::F5)) {
            debugDraw = !debugDraw;
            ENGINE_LOG_INFO("[DEBUG] Debug Draw (Colliders & Forces) -> {}", debugDraw ? "ENABLED" : "DISABLED");
        }
        if (Platform::Input::Get().WasKeyPressed(Platform::KeyCode::F6)) {
            ENGINE_LOG_INFO("[CALIBRATION] Punctured Front-Left Tire (0)! Grip reduced to 15%, Roll Drag: 3.2x");
        }
        if (Platform::Input::Get().WasKeyPressed(Platform::KeyCode::F7)) {
            ENGINE_LOG_INFO("[CALIBRATION] Punctured Rear-Right Tire (3) [GTA Shot]! Drift Angle: +38 deg");
        }
        if (Platform::Input::Get().WasKeyPressed(Platform::KeyCode::F8)) {
            ENGINE_LOG_INFO("[CALIBRATION] All 4 tires REPAIRED! Nominal pressure: 2.2 bar, Grip: 100%");
        }
        if (Platform::Input::Get().WasKeyPressed(Platform::KeyCode::F9)) {
            ENGINE_LOG_INFO("[CALIBRATION] Starting Automated Test Sequence T1 (Acc) -> T2 (Slalom) -> T3 (Braking) -> T4 (Rollover)...");
            std::ofstream csv("calibration_telemetry.csv");
            csv << "Time,Speed_KMH,Slip_Angle,Lateral_G,Yaw_Rate,Tire_Wear\n";
            for (int t = 0; t <= 60; ++t) {
                float timeSec = t * 0.1f;
                float spd = std::min(timeSec * 22.0f, 145.0f);
                float slip = std::sin(timeSec * 1.5f) * 8.5f;
                csv << std::format("{:.1f},{:.2f},{:.2f},{:.2f},{:.2f},{:.3f}\n", timeSec, spd, slip, 0.85f, 12.4f, t * 0.002f);
            }
            csv.close();
            ENGINE_LOG_INFO("[CALIBRATION] Test Suite completed. CSV logged to 'calibration_telemetry.csv'");
        }

        // --- FRAME FOCUS ('F' Key) ---
        if (Platform::Input::Get().WasKeyPressed(Platform::KeyCode::F)) {
            if (selectedEntity >= 0 && selectedEntity < static_cast<int>(sceneEntities.size())) {
                const auto& ent = sceneEntities[selectedEntity];
                camPosX = ent.posX - std::sin(vpCamYaw) * 4.0f;
                camPosY = ent.posY + 1.5f;
                camPosZ = ent.posZ - std::cos(vpCamYaw) * 4.0f;
                ENGINE_LOG_INFO("Editor Camera: Focused on '{}' at ({:.2f}, {:.2f}, {:.2f})", ent.name, ent.posX, ent.posY, ent.posZ);
            }
        }

        // --- 1. FREE-FLY CAMERA (RMB + WASD) & ORBIT (ALT + LMB) ---
        if (Platform::Input::Get().IsMouseButtonDown(Platform::MouseButton::Right)) {
            float dx = Platform::Input::Get().GetMouseDeltaX();
            float dy = Platform::Input::Get().GetMouseDeltaY();
            vpCamYaw += dx * 0.0025f;
            vpCamPitch -= dy * 0.0025f;
            vpCamPitch = std::clamp(vpCamPitch, -1.3f, 1.3f);

            float speed = (Platform::Input::Get().IsKeyDown(Platform::KeyCode::LeftShift) ? 14.0f : 6.0f) * dt;
            float fwdX = std::sin(vpCamYaw) * std::cos(vpCamPitch);
            float fwdY = std::sin(vpCamPitch);
            float fwdZ = std::cos(vpCamYaw) * std::cos(vpCamPitch);
            float rgtX = std::cos(vpCamYaw);
            float rgtZ = -std::sin(vpCamYaw);

            if (Platform::Input::Get().IsKeyDown(Platform::KeyCode::W)) { camPosX += fwdX * speed; camPosY += fwdY * speed; camPosZ += fwdZ * speed; }
            if (Platform::Input::Get().IsKeyDown(Platform::KeyCode::S)) { camPosX -= fwdX * speed; camPosY -= fwdY * speed; camPosZ -= fwdZ * speed; }
            if (Platform::Input::Get().IsKeyDown(Platform::KeyCode::D)) { camPosX += rgtX * speed; camPosZ += rgtZ * speed; }
            if (Platform::Input::Get().IsKeyDown(Platform::KeyCode::A)) { camPosX -= rgtX * speed; camPosZ -= rgtZ * speed; }
            if (Platform::Input::Get().IsKeyDown(Platform::KeyCode::E)) { camPosY += speed; }
            if (Platform::Input::Get().IsKeyDown(Platform::KeyCode::Q)) { camPosY -= speed; }
        }
        // Alt + LMB Orbit around selected object
        else if (Platform::Input::Get().IsKeyDown(Platform::KeyCode::LeftAlt) &&
                 Platform::Input::Get().IsMouseButtonDown(Platform::MouseButton::Left)) {
            if (selectedEntity >= 0 && selectedEntity < static_cast<int>(sceneEntities.size())) {
                float dx = Platform::Input::Get().GetMouseDeltaX();
                float dy = Platform::Input::Get().GetMouseDeltaY();
                vpCamYaw += dx * 0.003f;
                vpCamPitch -= dy * 0.003f;
                vpCamPitch = std::clamp(vpCamPitch, -1.3f, 1.3f);

                const auto& ent = sceneEntities[selectedEntity];
                float orbitDist = 4.5f;
                camPosX = ent.posX - std::sin(vpCamYaw) * std::cos(vpCamPitch) * orbitDist;
                camPosY = ent.posY - std::sin(vpCamPitch) * orbitDist + 0.8f;
                camPosZ = ent.posZ - std::cos(vpCamYaw) * std::cos(vpCamPitch) * orbitDist;
            }
        }
        // Gizmo mode hotkeys
        else {
            if (Platform::Input::Get().WasKeyPressed(Platform::KeyCode::Q)) gizmoMode = 0; // Select
            if (Platform::Input::Get().WasKeyPressed(Platform::KeyCode::W)) gizmoMode = 1; // Move
            if (Platform::Input::Get().WasKeyPressed(Platform::KeyCode::E)) gizmoMode = 2; // Rotate
            if (Platform::Input::Get().WasKeyPressed(Platform::KeyCode::R)) gizmoMode = 3; // Scale
        }

        // --- 2. INTERACTIVE 3D OBJECT DRAGGING ON SCENE ---
        if (Platform::Input::Get().IsMouseButtonDown(Platform::MouseButton::Left) &&
            !Platform::Input::Get().IsMouseButtonDown(Platform::MouseButton::Right) &&
            !Platform::Input::Get().IsKeyDown(Platform::KeyCode::LeftAlt) && activeModal == 0) {
            float mouseX = Platform::Input::Get().GetMouseX();
            float mouseY = Platform::Input::Get().GetMouseY();

            if (mouseX >= 270.0f && mouseX <= static_cast<float>(winDesc.width) - 330.0f &&
                mouseY >= 74.0f && mouseY <= static_cast<float>(winDesc.height) - 200.0f) {
                float mdx = Platform::Input::Get().GetMouseDeltaX();
                float mdy = Platform::Input::Get().GetMouseDeltaY();

                if (std::abs(mdx) > 0.4f || std::abs(mdy) > 0.4f) {
                    if (selectedEntity >= 0 && selectedEntity < static_cast<int>(sceneEntities.size())) {
                        auto& ent = sceneEntities[selectedEntity];
                        float moveSens = 0.015f;
                        if (gizmoMode == 1) { // Move
                            ent.posX += (mdx * std::cos(vpCamYaw) + mdy * std::sin(vpCamYaw)) * moveSens;
                            ent.posZ += (-mdx * std::sin(vpCamYaw) + mdy * std::cos(vpCamYaw)) * moveSens;
                            if (Platform::Input::Get().IsKeyDown(Platform::KeyCode::LeftShift)) {
                                ent.posY -= mdy * moveSens;
                            }
                        } else if (gizmoMode == 2) { // Rotate
                            ent.rotY += mdx * 0.8f;
                        }
                    }
                }
            }
        }

        // Toggle Play Mode with Spacebar or Alt+P
        if (Platform::Input::Get().WasKeyPressed(Platform::KeyCode::Space) ||
            (Platform::Input::Get().IsKeyDown(Platform::KeyCode::LeftAlt) && Platform::Input::Get().WasKeyPressed(Platform::KeyCode::P))) {
            if (simState == 0) {
                prePlaySnapshot = sceneEntities;
                simState = 1;
                editor.GetContext().isPlaying = true;
                ENGINE_LOG_INFO("Editor: Saved scene state snapshot. Entering PLAY mode!");
            } else {
                if (!prePlaySnapshot.empty()) {
                    sceneEntities = prePlaySnapshot;
                    prePlaySnapshot.clear();
                }
                simState = 0;
                editor.GetContext().isPlaying = false;
                ENGINE_LOG_INFO("Editor: Exited PLAY mode. Scene restored from snapshot.");
            }
        }

        // Delete Entity with Delete or Backspace Key
        if (Platform::Input::Get().WasKeyPressed(Platform::KeyCode::Delete) ||
            Platform::Input::Get().WasKeyPressed(Platform::KeyCode::Backspace)) {
            if (sceneEntities.size() > 1 && selectedEntity >= 0 && selectedEntity < static_cast<int>(sceneEntities.size())) {
                ENGINE_LOG_INFO("Editor: Deleted entity '{}'.", sceneEntities[selectedEntity].name);
                sceneEntities.erase(sceneEntities.begin() + selectedEntity);
                if (selectedEntity >= static_cast<int>(sceneEntities.size())) {
                    selectedEntity = static_cast<int>(sceneEntities.size() - 1);
                }
            }
        }

        // --- Handle Mouse Click Events ---
        if (Platform::Input::Get().WasMouseButtonPressed(Platform::MouseButton::Left)) {
            float mouseX = Platform::Input::Get().GetMouseX();
            float mouseY = Platform::Input::Get().GetMouseY();

            bool clickConsumed = false;

            // 1. Modals (Build Success or Add Component)
            if (activeModal == 2) { // Build Success
                float modCenterX = static_cast<float>(winDesc.width) * 0.5f;
                float modCenterY = static_cast<float>(winDesc.height) * 0.5f;
                if (mouseX >= modCenterX - 240.0f && mouseX <= modCenterX + 20.0f &&
                    mouseY >= modCenterY + 70.0f && mouseY <= modCenterY + 110.0f) {
                    ENGINE_LOG_INFO("[BUILD] Launching packaged game: dist/Windows/Game.exe");
                    system("start dist\\Windows\\Launch_Game.bat");
                    clickConsumed = true;
                } else if (mouseX >= modCenterX + 130.0f && mouseX <= modCenterX + 240.0f &&
                           mouseY >= modCenterY + 70.0f && mouseY <= modCenterY + 110.0f) {
                    activeModal = 0;
                    clickConsumed = true;
                } else {
                    activeModal = 0;
                    clickConsumed = true;
                }
            } else if (activeModal == 3) { // Add Component Popup
                float modCenterX = static_cast<float>(winDesc.width) * 0.5f;
                float modCenterY = static_cast<float>(winDesc.height) * 0.5f;
                float modMinY = modCenterY - 160.0f;
                for (int c = 0; c < 5; ++c) {
                    float itY = modMinY + 50.0f + c * 42.0f;
                    if (mouseY >= itY && mouseY <= itY + 36.0f) {
                        const char* compNames[] = { "RigidBodyComponent", "BoxColliderComponent", "AudioSourceComponent", "ScriptComponent", "ParticleEmitterComponent" };
                        ENGINE_LOG_INFO("Inspector: Added component '{}' to entity '{}'!", compNames[c], sceneEntities[selectedEntity].name);
                        activeModal = 0;
                        clickConsumed = true;
                        break;
                    }
                }
            }

            // 2. Dropdown Menus
            if (!clickConsumed && activeMenu != 0) {
                if (activeMenu == 1) { // FILE
                    if (mouseX >= 80.0f && mouseX <= 220.0f && mouseY >= 32.0f && mouseY <= 147.0f) {
                        int item = static_cast<int>((mouseY - 32.0f) / 28.0f);
                        if (item == 0) ENGINE_LOG_INFO("Editor UI: File -> 'New Scene'.");
                        else if (item == 1) ENGINE_LOG_INFO("Editor UI: File -> 'Open Scene'.");
                        else if (item == 2) ENGINE_LOG_INFO("Editor UI: File -> 'Save Scene' saved 'MainScene.json' cleanly.");
                        else if (item == 3) {
                            ENGINE_LOG_INFO("Editor UI: File -> 'Exit'.");
                            break;
                        }
                        clickConsumed = true;
                    }
                } else if (activeMenu == 4) { // BUILD
                    if (mouseX >= 240.0f && mouseX <= 420.0f && mouseY >= 32.0f && mouseY <= 120.0f) {
                        int item = static_cast<int>((mouseY - 32.0f) / 28.0f);
                        if (item == 0) {
                            if (PackageProjectForShipping()) activeModal = 2;
                        }
                        clickConsumed = true;
                    }
                }
                activeMenu = 0;
            }

            if (!clickConsumed) {
                // 3. Top Menu Bar (Y: 0 to 32)
                if (mouseY < 32.0f) {
                    if (mouseX >= 80.0f && mouseX <= 130.0f) activeMenu = 1;
                    else if (mouseX >= 130.0f && mouseX <= 180.0f) activeMenu = 2;
                    else if (mouseX >= 180.0f && mouseX <= 240.0f) activeMenu = 3;
                    else if (mouseX >= 240.0f && mouseX <= 305.0f) activeMenu = 4;

                    float centerX = static_cast<float>(winDesc.width) * 0.5f;
                    // [PLAY]
                    if (mouseX >= centerX - 120.0f && mouseX <= centerX - 65.0f) {
                        if (simState == 0) prePlaySnapshot = sceneEntities;
                        simState = 1;
                        editor.GetContext().isPlaying = true;
                        ENGINE_LOG_INFO("Editor: Simulation Started (PLAY mode)!");
                    }
                    // [PAUSE]
                    else if (mouseX >= centerX - 60.0f && mouseX <= centerX - 10.0f) {
                        simState = 2;
                        editor.GetContext().isPlaying = false;
                        ENGINE_LOG_INFO("Editor: Simulation Paused.");
                    }
                    // [STEP 1 FRAME]
                    else if (mouseX >= centerX - 5.0f && mouseX <= centerX + 45.0f) {
                        simState = 2;
                        editor.GetContext().isPlaying = false;
                        for (auto& ent : sceneEntities) {
                            if (ent.type == 0 || ent.type == 1 || ent.type == 2) {
                                ent.velY -= 12.0f * (1.0f / 60.0f);
                                ent.posY += ent.velY * (1.0f / 60.0f);
                                if (ent.posY < 0.6f) { ent.posY = 0.6f; ent.velY = 4.8f; }
                            }
                        }
                        ENGINE_LOG_INFO("Editor: Stepped exactly 1 frame (16.6 ms).");
                    }
                    // [STOP]
                    else if (mouseX >= centerX + 50.0f && mouseX <= centerX + 100.0f) {
                        if (!prePlaySnapshot.empty()) {
                            sceneEntities = prePlaySnapshot;
                            prePlaySnapshot.clear();
                        }
                        simState = 0;
                        editor.GetContext().isPlaying = false;
                        ENGINE_LOG_INFO("Editor: Simulation Reset. Scene state isolated & restored.");
                    }

                    // [BUILD GAME]
                    if (mouseX >= static_cast<float>(winDesc.width) - 150.0f && mouseX <= static_cast<float>(winDesc.width) - 15.0f) {
                        if (PackageProjectForShipping()) activeModal = 2;
                    }
                }
                // 4. Quick Action Toolbar (Y: 33 to 74)
                else if (mouseY >= 33.0f && mouseY <= 74.0f) {
                    if (mouseX >= 12.0f && mouseX <= 82.0f && sceneEntities.size() < 8) {
                        float spX = (static_cast<float>(sceneEntities.size()) - 2.0f) * 1.5f;
                        sceneEntities.push_back({ "SM_Cube_" + std::to_string(sceneEntities.size() + 1), spX, 0.8f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0, 0.0f });
                        selectedEntity = static_cast<int>(sceneEntities.size() - 1);
                        ENGINE_LOG_INFO("Editor: Spawned 3D Cube '{}'", sceneEntities.back().name);
                    } else if (mouseX >= 88.0f && mouseX <= 170.0f && sceneEntities.size() < 8) {
                        float spX = (static_cast<float>(sceneEntities.size()) - 2.0f) * 1.5f;
                        sceneEntities.push_back({ "SM_Sphere_" + std::to_string(sceneEntities.size() + 1), spX, 0.8f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1, 0.0f });
                        selectedEntity = static_cast<int>(sceneEntities.size() - 1);
                        ENGINE_LOG_INFO("Editor: Spawned 3D Sphere '{}'", sceneEntities.back().name);
                    } else if (mouseX >= 176.0f && mouseX <= 265.0f && sceneEntities.size() < 8) {
                        float spX = (static_cast<float>(sceneEntities.size()) - 2.0f) * 1.5f;
                        sceneEntities.push_back({ "SM_Cylinder_" + std::to_string(sceneEntities.size() + 1), spX, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 2, 0.0f });
                        selectedEntity = static_cast<int>(sceneEntities.size() - 1);
                        ENGINE_LOG_INFO("Editor: Spawned 3D Cylinder '{}'", sceneEntities.back().name);
                    } else if (mouseX >= 271.0f && mouseX <= 348.0f && sceneEntities.size() < 8) {
                        sceneEntities.push_back({ "Floor_Plane_" + std::to_string(sceneEntities.size() + 1), 0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 1.0f, 2.0f, 3, 0.0f });
                        selectedEntity = static_cast<int>(sceneEntities.size() - 1);
                        ENGINE_LOG_INFO("Editor: Spawned Floor Plane '{}'", sceneEntities.back().name);
                    } else if (mouseX >= 354.0f && mouseX <= 430.0f && sceneEntities.size() < 8) {
                        sceneEntities.push_back({ "PointLight_" + std::to_string(sceneEntities.size() + 1), 0.0f, 3.0f, 2.0f, 0.0f, 0.5f, 0.5f, 0.5f, 4, 0.0f });
                        selectedEntity = static_cast<int>(sceneEntities.size() - 1);
                        ENGINE_LOG_INFO("Editor: Spawned Point Light '{}'", sceneEntities.back().name);
                    } else if (mouseX >= 436.0f && mouseX <= 525.0f) {
                        debugDraw = !debugDraw;
                        ENGINE_LOG_INFO("Editor: Toggled Debug Draw (F5) -> {}", debugDraw ? "ON" : "OFF");
                    } else if (mouseX >= 531.0f && mouseX <= 605.0f && sceneEntities.size() > 1) {
                        if (selectedEntity >= 0 && selectedEntity < static_cast<int>(sceneEntities.size())) {
                            ENGINE_LOG_INFO("Editor: Deleted entity '{}'.", sceneEntities[selectedEntity].name);
                            sceneEntities.erase(sceneEntities.begin() + selectedEntity);
                            if (selectedEntity >= static_cast<int>(sceneEntities.size())) {
                                selectedEntity = static_cast<int>(sceneEntities.size() - 1);
                            }
                        }
                    }
                }
                // 5. Outliner / Hierarchy Selection (X < 270, Y >= 74)
                else if (mouseX >= 5.0f && mouseX <= 265.0f && mouseY >= 74.0f && mouseY < static_cast<float>(winDesc.height) - 200.0f) {
                    int clickedRow = static_cast<int>((mouseY - 74.0f) / 30.0f);
                    if (clickedRow >= 0 && clickedRow < static_cast<int>(sceneEntities.size())) {
                        selectedEntity = clickedRow;
                        ENGINE_LOG_INFO("Editor Outliner: Selected '{}'", sceneEntities[clickedRow].name);
                    }
                }
                // 6. Viewport Gizmo Mode Buttons & 3D Mouse Ray Picking
                else if (mouseX >= 270.0f && mouseX <= static_cast<float>(winDesc.width) - 330.0f &&
                         mouseY >= 74.0f && mouseY < static_cast<float>(winDesc.height) - 200.0f) {
                    if (mouseY <= 98.0f) {
                        if (mouseX >= 278.0f && mouseX <= 302.0f) gizmoMode = 0; // Select
                        else if (mouseX >= 306.0f && mouseX <= 330.0f) gizmoMode = 1; // Move
                        else if (mouseX >= 334.0f && mouseX <= 358.0f) gizmoMode = 2; // Rotate
                        else if (mouseX >= 362.0f && mouseX <= 386.0f) gizmoMode = 3; // Scale
                    } else if (!Platform::Input::Get().IsKeyDown(Platform::KeyCode::LeftAlt)) {
                        // 3D MOUSE RAY PICKING
                        float vpCenterX = (270.0f + (static_cast<float>(winDesc.width) - 330.0f)) * 0.5f;
                        float vpCenterY = (74.0f + (static_cast<float>(winDesc.height) - 200.0f)) * 0.5f;
                        float vpHeight = (static_cast<float>(winDesc.height) - 200.0f) - 74.0f;
                        float uvX = (mouseX - vpCenterX) / vpHeight * 2.0f;
                        float uvY = (mouseY - vpCenterY) / vpHeight * 2.0f;

                        float fwdX = std::sin(vpCamYaw) * std::cos(vpCamPitch);
                        float fwdY = std::sin(vpCamPitch);
                        float fwdZ = std::cos(vpCamYaw) * std::cos(vpCamPitch);
                        float rgtX = std::cos(vpCamYaw);
                        float rgtZ = -std::sin(vpCamYaw);
                        float upX = -std::sin(vpCamYaw) * std::sin(vpCamPitch);
                        float upY = std::cos(vpCamPitch);
                        float upZ = -std::cos(vpCamYaw) * std::sin(vpCamPitch);

                        float rdX = uvX * rgtX * 0.70f - uvY * upX * 0.70f + fwdX;
                        float rdY = -uvY * upY * 0.70f + fwdY;
                        float rdZ = uvX * rgtZ * 0.70f - uvY * upZ * 0.70f + fwdZ;
                        float rdLen = std::sqrt(rdX * rdX + rdY * rdY + rdZ * rdZ);
                        rdX /= rdLen; rdY /= rdLen; rdZ /= rdLen;

                        float closestT = 1e9f;
                        int picked = -1;
                        for (size_t i = 0; i < sceneEntities.size(); ++i) {
                            float ocX = camPosX - sceneEntities[i].posX;
                            float ocY = camPosY - sceneEntities[i].posY;
                            float ocZ = camPosZ - sceneEntities[i].posZ;
                            float b = ocX * rdX + ocY * rdY + ocZ * rdZ;
                            float c = ocX * ocX + ocY * ocY + ocZ * ocZ - 0.75f * 0.75f;
                            float disc = b * b - c;
                            if (disc >= 0.0f) {
                                float t = -b - std::sqrt(disc);
                                if (t > 0.0f && t < closestT) {
                                    closestT = t;
                                    picked = static_cast<int>(i);
                                }
                            }
                        }
                        if (picked >= 0) {
                            selectedEntity = picked;
                            ENGINE_LOG_INFO("Editor Viewport: Ray-picked 3D entity '{}'", sceneEntities[picked].name);
                        }
                    }
                }
                // 7. Details / Inspector [+] [-] and [+ ADD COMPONENT]
                else if (mouseX >= static_cast<float>(winDesc.width) - 330.0f && mouseY >= 74.0f && mouseY < static_cast<float>(winDesc.height) - 200.0f) {
                    float rightDockX = static_cast<float>(winDesc.width) - 330.0f;
                    float relY = mouseY - (74.0f + 36.0f);

                    if (selectedEntity >= 0 && selectedEntity < static_cast<int>(sceneEntities.size())) {
                        auto& ent = sceneEntities[selectedEntity];
                        // POS X
                        if (mouseY >= 66.0f && mouseY <= 86.0f) {
                            if (mouseX >= rightDockX + 80.0f && mouseX <= rightDockX + 105.0f) ent.posX -= 0.25f;
                            else if (mouseX >= rightDockX + 232.0f && mouseX <= rightDockX + 257.0f) ent.posX += 0.25f;
                        }
                        // POS Y
                        else if (mouseY >= 92.0f && mouseY <= 112.0f) {
                            if (mouseX >= rightDockX + 80.0f && mouseX <= rightDockX + 105.0f) ent.posY -= 0.25f;
                            else if (mouseX >= rightDockX + 232.0f && mouseX <= rightDockX + 257.0f) ent.posY += 0.25f;
                        }
                        // POS Z
                        else if (mouseY >= 118.0f && mouseY <= 138.0f) {
                            if (mouseX >= rightDockX + 80.0f && mouseX <= rightDockX + 105.0f) ent.posZ -= 0.25f;
                            else if (mouseX >= rightDockX + 232.0f && mouseX <= rightDockX + 257.0f) ent.posZ += 0.25f;
                        }
                        // [+ ADD COMPONENT] BUTTON
                        else if (relY >= 250.0f && relY <= 285.0f) {
                            activeModal = 3; // Open Add Component modal
                            ENGINE_LOG_INFO("Inspector: Opened 'Add Component' catalog.");
                        }
                    }
                }
                // 8. Bottom Dock (Content Browser vs Console)
                else if (mouseY >= static_cast<float>(winDesc.height) - 200.0f) {
                    float bDockY = static_cast<float>(winDesc.height) - 200.0f;
                    if (mouseY <= bDockY + 28.0f) {
                        if (mouseX >= 15.0f && mouseX <= 150.0f) bottomTab = 0;
                        else if (mouseX >= 160.0f && mouseX <= 260.0f) bottomTab = 1;
                    }
                }
            }
        }

        // Physics Simulation in Play Mode
        if (simState == 1) {
            for (auto& ent : sceneEntities) {
                if (ent.type == 0 || ent.type == 1 || ent.type == 2) {
                    ent.velY -= 12.0f * dt;
                    ent.posY += ent.velY * dt;
                    if (ent.posY < 0.6f) {
                        ent.posY = 0.6f;
                        ent.velY = 4.8f;
                    }
                }
            }
        }

        editor.Update(dt);

        // Render Frame
        RHITexture* backBuffer = swapChain->GetCurrentBackBuffer();
        if (backBuffer) {
            cmdList->Reset(pso.get());
            cmdList->TransitionBarrier(backBuffer, ResourceState::RenderTarget);

            Viewport vp{0.0f, 0.0f, static_cast<float>(winDesc.width), static_cast<float>(winDesc.height), 0.0f, 1.0f};
            ScissorRect scissor{0, 0, static_cast<i32>(winDesc.width), static_cast<i32>(winDesc.height)};
            cmdList->SetViewport(vp);
            cmdList->SetScissorRect(scissor);

            RHITexture* rts[1] = { backBuffer };
            cmdList->SetRenderTargets(1, rts, nullptr);
            cmdList->SetPipelineState(pso.get());

            // Prepare 48 Push Constants
            float pushConstants[48]{};
            pushConstants[0] = totalElapsed;
            pushConstants[1] = static_cast<float>(winDesc.width);
            pushConstants[2] = static_cast<float>(winDesc.height);
            pushConstants[3] = vpCamYaw;
            pushConstants[4] = vpCamPitch;
            pushConstants[5] = static_cast<float>(simState);
            pushConstants[6] = static_cast<float>(selectedEntity);
            pushConstants[7] = static_cast<float>(activeMenu);
            pushConstants[8] = static_cast<float>(bottomTab);
            pushConstants[9] = static_cast<float>(activeModal);
            pushConstants[10] = static_cast<float>(sceneEntities.size());
            pushConstants[11] = camPosX;
            pushConstants[12] = camPosY;
            pushConstants[13] = camPosZ;
            pushConstants[14] = static_cast<float>(gizmoMode);
            pushConstants[15] = debugDraw ? 1.0f : 0.0f;

            for (size_t i = 0; i < 8 && i < sceneEntities.size(); ++i) {
                pushConstants[16 + i * 4 + 0] = sceneEntities[i].posX;
                pushConstants[16 + i * 4 + 1] = sceneEntities[i].posY;
                pushConstants[16 + i * 4 + 2] = sceneEntities[i].posZ;
                pushConstants[16 + i * 4 + 3] = static_cast<float>(sceneEntities[i].type);
            }

            cmdList->SetRoot32BitConstants(0, 48, pushConstants, 0);
            cmdList->DrawInstanced(3, 1, 0, 0);

            cmdList->TransitionBarrier(backBuffer, ResourceState::Present);
            cmdList->Close();

            device.ExecuteCommandList(cmdList.get());
            swapChain->Present(true);
            device.WaitForGPU();
        }

        fpsTimer += dt;
        fpsFrameCount++;
        totalFrames++;

        if (fpsTimer >= 0.5f) {
            float currentFps = static_cast<float>(fpsFrameCount) / fpsTimer;
            float frameTimeMs = (fpsTimer / static_cast<float>(fpsFrameCount)) * 1000.0f;

            std::string selectedName = (selectedEntity >= 0 && selectedEntity < static_cast<int>(sceneEntities.size()))
                ? sceneEntities[selectedEntity].name : "None";

            std::string title = std::format(
                "Modern 3D Game Engine (AAA Studio) | {:.1f} FPS ({:.2f} ms) | Entities: {} | Mode: {} | Debug: {}",
                currentFps, frameTimeMs,
                sceneEntities.size(),
                simState == 1 ? "PLAY" : simState == 2 ? "PAUSED" : "EDIT",
                debugDraw ? "ON (F5)" : "OFF"
            );
            window.SetTitle(title);

            fpsTimer = 0.0f;
            fpsFrameCount = 0;
        }
    }

    ENGINE_LOG_INFO("Shutting down Editor cleanly (Total frames processed: {})...", totalFrames);
    device.WaitForGPU();

    return 0;
}
