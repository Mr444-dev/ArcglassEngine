#include "editor/EditorApp.h"
#include "engine/core/Log.h"
#include "engine/platform/Platform.h"
#include "engine/platform/Window.h"
#include "engine/platform/Input.h"
#include "engine/rhi/RHI.h"
#include "engine/rhi/RHIDevice.h"

#include <chrono>
#include <format>
#include <numbers>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <thread>

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <commdlg.h>
    #include <shellapi.h>
#endif

using namespace Engine;
using namespace Engine::Core;
using namespace Engine::RHI;

namespace {
bool LoadShaderBytecode(const std::filesystem::path& path, std::vector<u8>& outBytecode) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        ENGINE_LOG_ERROR("Failed to open precompiled shader: {}", path.string());
        return false;
    }

    const std::streamsize size = file.tellg();
    if (size <= 0) {
        ENGINE_LOG_ERROR("Precompiled shader is empty: {}", path.string());
        return false;
    }

    outBytecode.resize(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(outBytecode.data()), size)) {
        ENGINE_LOG_ERROR("Failed to read precompiled shader: {}", path.string());
        outBytecode.clear();
        return false;
    }

    return true;
}
} // namespace

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
    u32 componentMask = 0;
};

enum EntityComponent : u32 {
    ComponentRigidBody = 1u << 0u,
    ComponentBoxCollider = 1u << 1u,
    ComponentAudioSource = 1u << 2u,
    ComponentScript = 1u << 3u,
    ComponentParticleEmitter = 1u << 4u
};

struct Ray3 {
    float originX, originY, originZ;
    float directionX, directionY, directionZ;
};

bool IntersectBounds(const Ray3& ray, float halfX, float halfY, float halfZ, float& outDistance) {
    float nearDistance = 0.0f;
    float farDistance = std::numeric_limits<float>::max();
    const float origins[] = {ray.originX, ray.originY, ray.originZ};
    const float directions[] = {ray.directionX, ray.directionY, ray.directionZ};
    const float extents[] = {halfX, halfY, halfZ};
    for (int axis = 0; axis < 3; ++axis) {
        const float origin = origins[axis];
        const float direction = directions[axis];
        if (std::abs(direction) < 1e-6f) {
            if (origin < -extents[axis] || origin > extents[axis]) return false;
            continue;
        }
        float first = (-extents[axis] - origin) / direction;
        float second = (extents[axis] - origin) / direction;
        if (first > second) std::swap(first, second);
        nearDistance = std::max(nearDistance, first);
        farDistance = std::min(farDistance, second);
        if (nearDistance > farDistance) return false;
    }
    outDistance = nearDistance > 0.0f ? nearDistance : farDistance;
    return outDistance > 0.0f;
}

Ray3 MakeViewportRay(float mouseX, float mouseY, u32 screenWidth, u32 screenHeight,
                     float cameraX, float cameraY, float cameraZ, float yaw, float pitch) {
    constexpr float leftDockWidth = 270.0f;
    constexpr float rightDockWidth = 330.0f;
    constexpr float topBarHeight = 74.0f;
    constexpr float bottomDockHeight = 200.0f;
    const float minX = leftDockWidth;
    const float maxX = static_cast<float>(screenWidth) - rightDockWidth;
    const float minY = topBarHeight;
    const float maxY = static_cast<float>(screenHeight) - bottomDockHeight;
    const float viewHeight = maxY - minY;
    if (viewHeight <= 1.0f || mouseX < minX || mouseX > maxX || mouseY < minY || mouseY > maxY) {
        return {cameraX, cameraY, cameraZ, 0.0f, 0.0f, 1.0f};
    }

    const float centerX = (minX + maxX) * 0.5f;
    const float centerY = (minY + maxY) * 0.5f;
    const float uvX = (mouseX - centerX) / viewHeight * 2.0f;
    const float uvY = (mouseY - centerY) / viewHeight * 2.0f;

    const float forwardX = std::sin(yaw) * std::cos(pitch);
    const float forwardY = std::sin(pitch);
    const float forwardZ = std::cos(yaw) * std::cos(pitch);
    // Match the editor shader's cross(forward, worldUp) camera basis exactly.
    const float rightX = -std::cos(yaw);
    const float rightZ = std::sin(yaw);
    const float upX = std::sin(yaw) * std::sin(pitch);
    const float upY = std::cos(pitch);
    const float upZ = -std::cos(yaw) * std::sin(pitch);

    float directionX = forwardX + uvX * rightX * 0.70f - uvY * upX * 0.70f;
    float directionY = forwardY - uvY * upY * 0.70f;
    float directionZ = forwardZ + uvX * rightZ * 0.70f - uvY * upZ * 0.70f;
    const float length = std::sqrt(directionX * directionX + directionY * directionY + directionZ * directionZ);
    if (length > 1e-6f) {
        directionX /= length;
        directionY /= length;
        directionZ /= length;
    }
    return {cameraX, cameraY, cameraZ, directionX, directionY, directionZ};
}

bool IntersectEntity(const Ray3& worldRay, const EditorEntity& entity, float& outDistance) {
    const float c = std::cos(entity.rotY);
    const float s = std::sin(entity.rotY);
    const float deltaX = worldRay.originX - entity.posX;
    const float deltaY = worldRay.originY - entity.posY;
    const float deltaZ = worldRay.originZ - entity.posZ;
    const Ray3 localRay{
        c * deltaX - s * deltaZ, deltaY, s * deltaX + c * deltaZ,
        c * worldRay.directionX - s * worldRay.directionZ, worldRay.directionY,
        s * worldRay.directionX + c * worldRay.directionZ
    };
    const float scale = std::max(entity.scaleX, 0.05f);

    if (entity.type == 0) return IntersectBounds(localRay, 0.55f * scale, 0.55f * scale, 0.55f * scale, outDistance);
    if (entity.type == 4) return IntersectBounds(localRay, 0.25f * scale, 0.25f * scale, 0.25f * scale, outDistance);
    if (entity.type == 1) {
        const float b = localRay.originX * localRay.directionX + localRay.originY * localRay.directionY + localRay.originZ * localRay.directionZ;
        const float radius = 0.60f * scale;
        const float cTerm = localRay.originX * localRay.originX + localRay.originY * localRay.originY + localRay.originZ * localRay.originZ - radius * radius;
        const float discriminant = b * b - cTerm;
        if (discriminant < 0.0f) return false;
        const float root = std::sqrt(discriminant);
        outDistance = -b - root;
        if (outDistance <= 0.0f) outDistance = -b + root;
        return outDistance > 0.0f;
    }
    if (entity.type == 2) {
        const float radius = 0.50f * scale;
        const float halfHeight = 0.70f * scale;
        const float a = localRay.directionX * localRay.directionX + localRay.directionZ * localRay.directionZ;
        const float b = 2.0f * (localRay.originX * localRay.directionX + localRay.originZ * localRay.directionZ);
        const float cTerm = localRay.originX * localRay.originX + localRay.originZ * localRay.originZ - radius * radius;
        const float discriminant = b * b - 4.0f * a * cTerm;
        float closest = std::numeric_limits<float>::max();
        if (a > 1e-6f && discriminant >= 0.0f) {
            const float root = std::sqrt(discriminant);
            for (float t : {(-b - root) / (2.0f * a), (-b + root) / (2.0f * a)}) {
                const float y = localRay.originY + localRay.directionY * t;
                if (t > 0.0f && std::abs(y) <= halfHeight) closest = std::min(closest, t);
            }
        }
        if (std::abs(localRay.directionY) > 1e-6f) {
            for (float capY : {-halfHeight, halfHeight}) {
                const float t = (capY - localRay.originY) / localRay.directionY;
                const float x = localRay.originX + localRay.directionX * t;
                const float z = localRay.originZ + localRay.directionZ * t;
                if (t > 0.0f && x * x + z * z <= radius * radius) closest = std::min(closest, t);
            }
        }
        if (closest == std::numeric_limits<float>::max()) return false;
        outDistance = closest;
        return true;
    }
    if (entity.type == 3) {
        if (std::abs(localRay.directionY) <= 1e-6f) return false;
        const float t = -localRay.originY / localRay.directionY;
        if (t <= 0.0f) return false;
        const float x = localRay.originX + localRay.directionX * t;
        const float z = localRay.originZ + localRay.directionZ * t;
        if (std::abs(x) > 0.5f * scale || std::abs(z) > 0.5f * scale) return false;
        outDistance = t;
        return true;
    }
    return false;
}

int PickEntity(const Ray3& ray, const std::vector<EditorEntity>& entities) {
    float closestDistance = std::numeric_limits<float>::max();
    int picked = -1;
    for (size_t index = 0; index < entities.size(); ++index) {
        float distance = 0.0f;
        if (IntersectEntity(ray, entities[index], distance) && distance < closestDistance) {
            closestDistance = distance;
            picked = static_cast<int>(index);
        }
    }
    return picked;
}

std::optional<std::filesystem::path> ChooseSceneFile(void* ownerHandle, bool save) {
#if defined(_WIN32)
    wchar_t fileName[MAX_PATH]{};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = static_cast<HWND>(ownerHandle);
    dialog.lpstrFilter = L"Arcglass Scene (*.arcscene)\0*.arcscene\0Legacy MGE Scene (*.mgescene)\0*.mgescene\0All Files (*.*)\0*.*\0\0";
    dialog.nFilterIndex = 1;
    dialog.lpstrFile = fileName;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrDefExt = L"arcscene";
    dialog.Flags = OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST |
        (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    const BOOL accepted = save ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog);
    if (accepted) return std::filesystem::path(fileName);
#else
    (void)ownerHandle;
    (void)save;
#endif
    return std::nullopt;
}

bool SaveSceneFile(const std::filesystem::path& path, const std::vector<EditorEntity>& entities) {
    if (entities.size() > 8) {
        ENGINE_LOG_ERROR("Cannot save scene: the editor currently supports at most 8 entities.");
        return false;
    }
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        ENGINE_LOG_ERROR("Cannot open scene file for writing: {}", path.string());
        return false;
    }
    file << "MGE_SCENE 1\n" << entities.size() << '\n' << std::setprecision(9);
    for (const auto& entity : entities) {
        file << std::quoted(entity.name) << ' '
             << entity.posX << ' ' << entity.posY << ' ' << entity.posZ << ' '
             << entity.rotY << ' ' << entity.scaleX << ' ' << entity.scaleY << ' ' << entity.scaleZ << ' '
             << entity.type << ' ' << entity.velY << ' ' << entity.componentMask << '\n';
    }
    if (!file) {
        ENGINE_LOG_ERROR("Failed while writing scene file: {}", path.string());
        return false;
    }
    ENGINE_LOG_INFO("Saved {} scene entities to '{}'.", entities.size(), path.string());
    return true;
}

bool LoadSceneFile(const std::filesystem::path& path, std::vector<EditorEntity>& entities) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        ENGINE_LOG_ERROR("Cannot open scene file: {}", path.string());
        return false;
    }
    std::string magic;
    unsigned version = 0;
    size_t entityCount = 0;
    if (!(file >> magic >> version >> entityCount) || magic != "MGE_SCENE" || version != 1 || entityCount > 8) {
        ENGINE_LOG_ERROR("Invalid or unsupported scene header in '{}'.", path.string());
        return false;
    }

    std::vector<EditorEntity> loaded;
    loaded.reserve(entityCount);
    for (size_t index = 0; index < entityCount; ++index) {
        EditorEntity entity{};
        if (!(file >> std::quoted(entity.name)
                   >> entity.posX >> entity.posY >> entity.posZ
                   >> entity.rotY >> entity.scaleX >> entity.scaleY >> entity.scaleZ
                   >> entity.type >> entity.velY >> entity.componentMask)) {
            ENGINE_LOG_ERROR("Malformed entity {} in '{}'.", index, path.string());
            return false;
        }
        const float values[] = {entity.posX, entity.posY, entity.posZ, entity.rotY,
            entity.scaleX, entity.scaleY, entity.scaleZ, entity.velY};
        if (entity.name.empty() || entity.name.size() > 128 || entity.type < 0 || entity.type > 4 ||
            std::any_of(std::begin(values), std::end(values), [](float value) { return !std::isfinite(value); }) ||
            entity.scaleX <= 0.0f || entity.scaleY <= 0.0f || entity.scaleZ <= 0.0f ||
            (entity.componentMask & ~0x1Fu) != 0u) {
            ENGINE_LOG_ERROR("Entity {} contains invalid values in '{}'.", index, path.string());
            return false;
        }
        loaded.push_back(std::move(entity));
    }
    std::string trailingData;
    if (file >> trailingData) {
        ENGINE_LOG_ERROR("Unexpected data after scene entities in '{}'.", path.string());
        return false;
    }
    entities = std::move(loaded);
    ENGINE_LOG_INFO("Loaded {} scene entities from '{}'.", entities.size(), path.string());
    return true;
}

// Automated Standalone Shipping Packager
bool PackageProjectForShipping(const std::filesystem::path& runtimeExecutable, std::filesystem::path& outputDirectory) {
    ENGINE_LOG_INFO("[BUILD] Packaging the Windows runtime and its required shaders...");
    try {
        if (!std::filesystem::is_regular_file(runtimeExecutable)) {
            ENGINE_LOG_ERROR("[BUILD] Runtime executable not found: {}", runtimeExecutable.string());
            return false;
        }

        const auto runtimeShaderSource = runtimeExecutable.parent_path() / "shaders" / "runtime";
        const auto vertexShader = runtimeShaderSource / "RuntimeScene.vs_5_1.cso";
        const auto pixelShader = runtimeShaderSource / "RuntimeScene.ps_5_1.cso";
        if (!std::filesystem::is_regular_file(vertexShader) || !std::filesystem::is_regular_file(pixelShader)) {
            ENGINE_LOG_ERROR("[BUILD] Runtime shader files are missing beside {}. Build the runtime first.", runtimeExecutable.string());
            return false;
        }

        std::filesystem::path sourceRoot;
        for (auto candidate = runtimeExecutable.parent_path(); !candidate.empty(); candidate = candidate.parent_path()) {
            if (std::filesystem::is_regular_file(candidate / "CMakeLists.txt") &&
                std::filesystem::is_directory(candidate / "runtime")) {
                sourceRoot = candidate;
                break;
            }
            if (candidate == candidate.parent_path()) break;
        }
        if (sourceRoot.empty()) sourceRoot = runtimeExecutable.parent_path();
        outputDirectory = sourceRoot / "dist" / "Windows";
        std::filesystem::create_directories(outputDirectory / "shaders" / "runtime");
        std::filesystem::copy_file(runtimeExecutable, outputDirectory / "Game.exe",
            std::filesystem::copy_options::overwrite_existing);
        std::filesystem::copy_file(vertexShader, outputDirectory / "shaders" / "runtime" / vertexShader.filename(),
            std::filesystem::copy_options::overwrite_existing);
        std::filesystem::copy_file(pixelShader, outputDirectory / "shaders" / "runtime" / pixelShader.filename(),
            std::filesystem::copy_options::overwrite_existing);

        const auto assets = sourceRoot / "assets";
        if (std::filesystem::is_directory(assets)) {
            std::filesystem::copy(assets, outputDirectory / "assets",
                std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
        }

        std::ofstream out(outputDirectory / "Launch_Game.bat", std::ios::binary | std::ios::trunc);
        if (!out) {
            ENGINE_LOG_ERROR("[BUILD] Could not create the launcher in {}.", outputDirectory.string());
            return false;
        }
        out << "@echo off\n";
        out << "start \"\" \"%~dp0Game.exe\"\n";
        out.close();
        if (!out) {
            ENGINE_LOG_ERROR("[BUILD] Failed to finish writing the launcher.");
            return false;
        }

        ENGINE_LOG_INFO("[BUILD] Standalone game and shaders packaged at {}.", outputDirectory.string());
        return true;
    } catch (const std::exception& e) {
        ENGINE_LOG_ERROR("[BUILD] Packaging failed: {}", e.what());
        return false;
    }
}

int main(int argc, char* argv[]) {
    (void)argc;

    ENGINE_LOG_INFO("=================================================");
    ENGINE_LOG_INFO("   Arcglass Engine Editor  ");
    ENGINE_LOG_INFO("=================================================");

    // 1. Initialize Platform layer and main Editor window
    Platform::Initialize();

    Platform::Window window;
    Platform::WindowDesc winDesc{};
    winDesc.title = "Arcglass Engine Editor";
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

    window.SetResizeCallback([&](u32 width, u32 height) {
        device.WaitForGPU();
        swapChain->Resize(width, height);
        winDesc.width = width;
        winDesc.height = height;
    });

    auto cmdList = device.CreateCommandList();
    if (!cmdList) {
        ENGINE_LOG_ERROR("Failed to create CommandList for editor.");
        device.Shutdown();
        Platform::Shutdown();
        return 1;
    }

    // 3. Load editor UI shaders compiled by the CMake build.
    window.SetTitle("Arcglass Engine Editor | Loading shaders...");
    ENGINE_LOG_INFO("Loading precompiled editor shaders.");
    std::vector<u8> vsBytecode;
    std::vector<u8> psBytecode;
    const auto executableDirectory = std::filesystem::absolute(argv[0]).parent_path();
    std::filesystem::path packageDirectory;
    const auto runtimeExecutable = executableDirectory / "GameRuntime.exe";
    const auto editorShaderDirectory = executableDirectory / "shaders" / "editor";
    const bool vsOk = LoadShaderBytecode(editorShaderDirectory / "EditorUI.vs_5_1.cso", vsBytecode);
    const bool psOk = LoadShaderBytecode(editorShaderDirectory / "EditorUI.ps_5_1.cso", psBytecode);
    if (!vsOk || !psOk) {
        ENGINE_LOG_ERROR("Failed to load editor shader bytecode. Rebuild EngineEditor with CMake.");
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

    ENGINE_LOG_INFO("Arcglass Engine Editor ready. Fly: RMB+WASD. Focus: F. Orbit: Alt+LMB. Snap: 0.5m. Debug: F5.");

    auto prevTime = std::chrono::high_resolution_clock::now();
    float fpsTimer = 0.0f;
    u32 fpsFrameCount = 0;
    u64 totalFrames = 0;
    float totalElapsed = 0.0f;

    // Viewport Free-Fly Camera (WASD + RMB)
    float camPosX = 0.0f;
    float camPosY = 2.4f;
    float camPosZ = 5.5f;
    // The default camera is at +Z and must look back toward the origin.
    float vpCamYaw = std::numbers::pi_v<float>;
    float vpCamPitch = -0.22f;

    // Simulation States: 0 = EDIT, 1 = PLAY, 2 = PAUSE
    int simState = 0;
    std::vector<EditorEntity> prePlaySnapshot;

    // Interactive UI State
    int activeMenu = 0;       // 0=None, 1=File, 2=Edit, 3=Scene, 4=Build
    int bottomTab = 0;        // 0=Create, 1=Shortcuts
    int activeModal = 0;      // 0=None, 1=Settings, 2=BuildSuccess, 3=AddComponent
    int gizmoMode = 1;        // 0=Select, 1=Move(W), 2=Rotate(E), 3=Scale(R)
    bool debugDraw = false;   // F5 Debug drawing toggle

    // Undo / Redo history stacks
    std::vector<std::vector<EditorEntity>> undoStack;
    std::vector<std::vector<EditorEntity>> redoStack;

    // Scene Entities
    std::vector<EditorEntity> sceneEntities = {
        { "SM_Cube_01",      0.0f, 0.8f,  0.0f,  0.0f, 1.0f, 1.0f, 1.0f, 0, 0.0f, ComponentRigidBody | ComponentBoxCollider },
        { "SM_Sphere_01",    2.2f, 0.9f,  0.5f,  0.0f, 1.0f, 1.0f, 1.0f, 1, 0.0f, ComponentRigidBody | ComponentBoxCollider },
        { "SM_Cylinder_01", -2.2f, 1.0f,  0.5f,  0.0f, 1.0f, 1.0f, 1.0f, 2, 0.0f, ComponentRigidBody | ComponentBoxCollider },
        { "Floor_Plane",     0.0f, 0.0f,  0.0f,  0.0f, 3.0f, 1.0f, 3.0f, 3, 0.0f, ComponentBoxCollider },
        { "Sun_DirectionalLight",      -3.0f, 4.0f, -2.0f,  0.0f, 0.5f, 0.5f, 0.5f, 4, 0.0f, 0u }
    };
    int selectedEntity = 0;
    int draggingEntity = -1;
    bool dragUndoRecorded = false;
    std::vector<EditorEntity> dragUndoSnapshot;
    std::filesystem::path currentScenePath;
    bool sceneDirty = false;

    auto pushUndoSnapshot = [&]() {
        constexpr size_t maxHistory = 64;
        if (undoStack.size() >= maxHistory) undoStack.erase(undoStack.begin());
        undoStack.push_back(sceneEntities);
        redoStack.clear();
        sceneDirty = true;
    };
    auto doUndo = [&]() {
        if (undoStack.empty()) return;
        if (redoStack.size() >= 64) redoStack.erase(redoStack.begin());
        redoStack.push_back(sceneEntities);
        sceneEntities = std::move(undoStack.back());
        undoStack.pop_back();
        selectedEntity = sceneEntities.empty() ? -1 : std::clamp(selectedEntity, 0, static_cast<int>(sceneEntities.size()) - 1);
        sceneDirty = true;
    };
    auto doRedo = [&]() {
        if (redoStack.empty()) return;
        if (undoStack.size() >= 64) undoStack.erase(undoStack.begin());
        undoStack.push_back(sceneEntities);
        sceneEntities = std::move(redoStack.back());
        redoStack.pop_back();
        selectedEntity = sceneEntities.empty() ? -1 : std::clamp(selectedEntity, 0, static_cast<int>(sceneEntities.size()) - 1);
        sceneDirty = true;
    };
    auto saveCurrentScene = [&]() {
        auto path = currentScenePath;
        if (path.empty()) {
            const auto chosen = ChooseSceneFile(window.GetNativeHandle(), true);
            if (!chosen) return false;
            path = *chosen;
        }
        if (!SaveSceneFile(path, sceneEntities)) return false;
        currentScenePath = path;
        sceneDirty = false;
        return true;
    };
    auto openScene = [&]() {
        const auto chosen = ChooseSceneFile(window.GetNativeHandle(), false);
        if (!chosen) return false;
        std::vector<EditorEntity> loaded;
        if (!LoadSceneFile(*chosen, loaded)) return false;
        pushUndoSnapshot();
        sceneEntities = std::move(loaded);
        selectedEntity = sceneEntities.empty() ? -1 : 0;
        currentScenePath = *chosen;
        sceneDirty = false;
        return true;
    };
    auto packageGame = [&]() {
        return PackageProjectForShipping(runtimeExecutable, packageDirectory);
    };
    auto addEntity = [&](int type) {
        if (sceneEntities.size() >= 8 || type < 0 || type > 4) return false;
        pushUndoSnapshot();
        const size_t ordinal = sceneEntities.size() + 1;
        const char* typeNames[] = {"Cube", "Sphere", "Cylinder", "Plane", "DirectionalLight"};
        const float x = (static_cast<float>((ordinal - 1) % 3) - 1.0f) * 1.5f;
        const float z = static_cast<float>((ordinal - 1) / 3) * 1.5f;
        const float y = type == 3 ? 0.0f : type == 4 ? 3.0f : 0.8f;
        const float scale = type == 3 ? 3.0f : type == 4 ? 0.5f : 1.0f;
        const u32 components = type <= 2 ? ComponentRigidBody | ComponentBoxCollider : type == 3 ? ComponentBoxCollider : 0u;
        sceneEntities.push_back({std::format("SM_{}_{:02}", typeNames[type], ordinal), x, y, z, 0.0f,
            scale, type == 3 ? 1.0f : scale, scale, type, 0.0f, components});
        selectedEntity = static_cast<int>(sceneEntities.size()) - 1;
        draggingEntity = -1;
        return true;
    };
    auto deleteSelectedEntity = [&]() {
        if (selectedEntity < 0 || selectedEntity >= static_cast<int>(sceneEntities.size())) return false;
        pushUndoSnapshot();
        sceneEntities.erase(sceneEntities.begin() + selectedEntity);
        selectedEntity = sceneEntities.empty() ? -1 : std::min(selectedEntity, static_cast<int>(sceneEntities.size()) - 1);
        draggingEntity = -1;
        return true;
    };

    // Interactive Application Loop
    while (true) {
        Platform::Input::Get().BeginFrame();
        if (!window.PollEvents()) {
            break;
        }
        Platform::Input::Get().EndFrame();
        if (window.IsMinimized()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }

        if (Platform::Input::Get().WasKeyPressed(Platform::KeyCode::Escape)) {
            if (activeModal != 0) activeModal = 0;
            else if (activeMenu != 0) activeMenu = 0;
            else break;
        }

        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> delta = currentTime - prevTime;
        prevTime = currentTime;
        float dt = std::min(delta.count(), 0.1f);
        totalElapsed += dt;

        // --- EDITOR COMMANDS ---
        if (Platform::Input::Get().WasKeyPressed(Platform::KeyCode::F5)) {
            debugDraw = !debugDraw;
            ENGINE_LOG_INFO("Debug collider view {}.", debugDraw ? "enabled" : "disabled");
        }
        const bool controlDown = Platform::Input::Get().IsKeyDown(Platform::KeyCode::LeftControl) ||
                                 Platform::Input::Get().IsKeyDown(Platform::KeyCode::RightControl);
        if (controlDown && Platform::Input::Get().WasKeyPressed(Platform::KeyCode::S)) {
            saveCurrentScene();
        } else if (controlDown && Platform::Input::Get().WasKeyPressed(Platform::KeyCode::O)) {
            openScene();
        } else if (controlDown && Platform::Input::Get().WasKeyPressed(Platform::KeyCode::Z)) {
            doUndo();
        } else if (controlDown && Platform::Input::Get().WasKeyPressed(Platform::KeyCode::Y)) {
            doRedo();
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
        const float mouseX = Platform::Input::Get().GetMouseX();
        const float mouseY = Platform::Input::Get().GetMouseY();
        const bool mouseInViewport = mouseX >= 270.0f && mouseX <= static_cast<float>(winDesc.width) - 330.0f &&
            mouseY >= 98.0f && mouseY < static_cast<float>(winDesc.height) - 200.0f;
        if (Platform::Input::Get().IsMouseButtonDown(Platform::MouseButton::Right) && mouseInViewport) {
            float dx = Platform::Input::Get().GetMouseDeltaX();
            float dy = Platform::Input::Get().GetMouseDeltaY();
            vpCamYaw += dx * 0.0025f;
            vpCamPitch -= dy * 0.0025f;
            vpCamPitch = std::clamp(vpCamPitch, -1.3f, 1.3f);

            float speed = (Platform::Input::Get().IsKeyDown(Platform::KeyCode::LeftShift) ? 14.0f : 6.0f) * dt;
            float fwdX = std::sin(vpCamYaw) * std::cos(vpCamPitch);
            float fwdY = std::sin(vpCamPitch);
            float fwdZ = std::cos(vpCamYaw) * std::cos(vpCamPitch);
            float rgtX = -std::cos(vpCamYaw);
            float rgtZ = std::sin(vpCamYaw);

            if (Platform::Input::Get().IsKeyDown(Platform::KeyCode::W)) { camPosX += fwdX * speed; camPosY += fwdY * speed; camPosZ += fwdZ * speed; }
            if (Platform::Input::Get().IsKeyDown(Platform::KeyCode::S)) { camPosX -= fwdX * speed; camPosY -= fwdY * speed; camPosZ -= fwdZ * speed; }
            if (Platform::Input::Get().IsKeyDown(Platform::KeyCode::D)) { camPosX += rgtX * speed; camPosZ += rgtZ * speed; }
            if (Platform::Input::Get().IsKeyDown(Platform::KeyCode::A)) { camPosX -= rgtX * speed; camPosZ -= rgtZ * speed; }
            if (Platform::Input::Get().IsKeyDown(Platform::KeyCode::E)) { camPosY += speed; }
            if (Platform::Input::Get().IsKeyDown(Platform::KeyCode::Q)) { camPosY -= speed; }
        }
        // Alt + LMB Orbit around selected object
        else if (mouseInViewport && Platform::Input::Get().IsKeyDown(Platform::KeyCode::LeftAlt) &&
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

        if (Platform::Input::Get().WasMouseButtonPressed(Platform::MouseButton::Left)) {
            draggingEntity = -1;
            dragUndoRecorded = false;
        }

        // --- 2. INTERACTIVE 3D OBJECT DRAGGING ON SCENE ---
        if (Platform::Input::Get().IsMouseButtonDown(Platform::MouseButton::Left) &&
            !Platform::Input::Get().IsMouseButtonDown(Platform::MouseButton::Right) &&
            !Platform::Input::Get().IsKeyDown(Platform::KeyCode::LeftAlt) && activeModal == 0 &&
            simState == 0 && mouseInViewport && draggingEntity >= 0 && draggingEntity == selectedEntity &&
            draggingEntity < static_cast<int>(sceneEntities.size())) {
                float mdx = Platform::Input::Get().GetMouseDeltaX();
                float mdy = Platform::Input::Get().GetMouseDeltaY();

                if (std::abs(mdx) > 0.4f || std::abs(mdy) > 0.4f) {
                    if (gizmoMode != 0) {
                        if (!dragUndoRecorded) {
                            if (undoStack.size() >= 64) undoStack.erase(undoStack.begin());
                            undoStack.push_back(dragUndoSnapshot);
                            redoStack.clear();
                            dragUndoRecorded = true;
                        }
                        auto& ent = sceneEntities[draggingEntity];
                        const float moveSensitivity = 0.006f;
                        if (gizmoMode == 1) {
                            const float rightX = -std::cos(vpCamYaw);
                            const float rightZ = std::sin(vpCamYaw);
                            if (Platform::Input::Get().IsKeyDown(Platform::KeyCode::LeftShift)) {
                                ent.posX += rightX * mdx * moveSensitivity;
                                ent.posZ += rightZ * mdx * moveSensitivity;
                                ent.posY -= mdy * moveSensitivity;
                            } else {
                                ent.posX += (rightX * mdx + std::sin(vpCamYaw) * mdy) * moveSensitivity;
                                ent.posZ += (rightZ * mdx + std::cos(vpCamYaw) * mdy) * moveSensitivity;
                            }
                        } else if (gizmoMode == 2) {
                            ent.rotY += mdx * 0.01f;
                        } else if (gizmoMode == 3) {
                            const float scaleDelta = (mdx - mdy) * 0.005f;
                            const float scale = std::clamp(ent.scaleX + scaleDelta, 0.1f, 20.0f);
                            ent.scaleX = scale;
                            ent.scaleY = scale;
                            ent.scaleZ = scale;
                        }
                        sceneDirty = true;
                    }
                }
            }
        if (Platform::Input::Get().WasMouseButtonReleased(Platform::MouseButton::Left)) {
            draggingEntity = -1;
            dragUndoRecorded = false;
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
            if (!sceneEntities.empty() && selectedEntity >= 0 && selectedEntity < static_cast<int>(sceneEntities.size())) {
                deleteSelectedEntity();
            }
        }

        // --- Handle Mouse Click Events ---
        if (Platform::Input::Get().WasMouseButtonPressed(Platform::MouseButton::Left)) {
            bool clickConsumed = false;

            // 1. Modals (Build Success or Add Component)
            if (activeModal == 2) { // Build Success
                float modCenterX = static_cast<float>(winDesc.width) * 0.5f;
                float modCenterY = static_cast<float>(winDesc.height) * 0.5f;
                if (mouseX >= modCenterX - 240.0f && mouseX <= modCenterX + 20.0f &&
                    mouseY >= modCenterY + 70.0f && mouseY <= modCenterY + 110.0f) {
#if defined(_WIN32)
                    const auto gameExecutable = packageDirectory / "Game.exe";
                    const auto launchResult = ShellExecuteW(static_cast<HWND>(window.GetNativeHandle()), L"open",
                        gameExecutable.c_str(), nullptr, packageDirectory.c_str(), SW_SHOWNORMAL);
                    if (reinterpret_cast<INT_PTR>(launchResult) <= 32) {
                        ENGINE_LOG_ERROR("Could not launch packaged runtime at {}.", gameExecutable.string());
                    }
#endif
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
                float modCenterY = static_cast<float>(winDesc.height) * 0.5f;
                float modMinY = modCenterY - 160.0f;
                for (int c = 0; c < 5; ++c) {
                    float itY = modMinY + 50.0f + c * 42.0f;
                    if (mouseX >= static_cast<float>(winDesc.width) * 0.5f - 205.0f &&
                        mouseX <= static_cast<float>(winDesc.width) * 0.5f + 205.0f &&
                        mouseY >= itY && mouseY <= itY + 36.0f) {
                        constexpr u32 componentBits[] = {ComponentRigidBody, ComponentBoxCollider, ComponentAudioSource,
                            ComponentScript, ComponentParticleEmitter};
                        constexpr const char* componentNames[] = {"RigidBody", "BoxCollider", "AudioSource", "Script", "ParticleEmitter"};
                        if (selectedEntity >= 0 && selectedEntity < static_cast<int>(sceneEntities.size())) {
                            auto& entity = sceneEntities[selectedEntity];
                            if ((entity.componentMask & componentBits[c]) == 0u) {
                                pushUndoSnapshot();
                                entity.componentMask |= componentBits[c];
                                ENGINE_LOG_INFO("Added {} to '{}'.", componentNames[c], entity.name);
                            } else {
                                ENGINE_LOG_INFO("'{}' already has {}.", entity.name, componentNames[c]);
                            }
                        }
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
                        if (item == 0) {
                            if (sceneDirty) {
#if defined(_WIN32)
                                const int discard = MessageBoxW(static_cast<HWND>(window.GetNativeHandle()),
                                    L"Discard the current unsaved scene and create a new scene?", L"Unsaved scene",
                                    MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
                                if (discard != IDYES) { clickConsumed = true; activeMenu = 0; continue; }
#endif
                            }
                            pushUndoSnapshot();
                            sceneEntities.clear();
                            selectedEntity = -1;
                            draggingEntity = -1;
                            currentScenePath.clear();
                            sceneDirty = true;
                            ENGINE_LOG_INFO("Created an empty scene.");
                        } else if (item == 1) {
                            if (openScene()) {
                                draggingEntity = -1;
                                ENGINE_LOG_INFO("Opened scene '{}'.", currentScenePath.string());
                            }
                        } else if (item == 2) {
                            saveCurrentScene();
                        }
                        else if (item == 3) {
                            ENGINE_LOG_INFO("Editor UI: File -> 'Exit'.");
                            break;
                        }
                        clickConsumed = true;
                    }
                } else if (activeMenu == 4) { // BUILD
                    if (mouseX >= 240.0f && mouseX <= 420.0f && mouseY >= 32.0f && mouseY <= 60.0f) {
                        if (packageGame()) activeModal = 2;
                        clickConsumed = true;
                    }
                } else if (activeMenu == 2) { // EDIT
                    if (mouseX >= 130.0f && mouseX <= 260.0f && mouseY >= 32.0f && mouseY <= 116.0f) {
                        const int item = static_cast<int>((mouseY - 32.0f) / 28.0f);
                        if (item == 0) doUndo();
                        else if (item == 1) doRedo();
                        else if (item == 2) deleteSelectedEntity();
                        clickConsumed = true;
                    }
                } else if (activeMenu == 3) { // SCENE
                    if (mouseX >= 180.0f && mouseX <= 320.0f && mouseY >= 32.0f && mouseY <= 116.0f) {
                        const int item = static_cast<int>((mouseY - 32.0f) / 28.0f);
                        if (item >= 0 && item <= 2 && !addEntity(item)) {
                            ENGINE_LOG_WARN("Scene is full; remove an entity before adding another.");
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
                        pushUndoSnapshot();
                        simState = 2;
                        editor.GetContext().isPlaying = false;
                        for (auto& ent : sceneEntities) {
                            if ((ent.componentMask & ComponentRigidBody) != 0u && ent.type <= 2) {
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
                        if (packageGame()) activeModal = 2;
                    }
                }
                // 4. Quick Action Toolbar (Y: 33 to 74)
                else if (mouseY >= 33.0f && mouseY <= 74.0f) {
                    if (mouseX >= 12.0f && mouseX <= 82.0f) {
                        if (!addEntity(0)) ENGINE_LOG_WARN("Scene is full; remove an entity before adding another.");
                    } else if (mouseX >= 88.0f && mouseX <= 170.0f) {
                        if (!addEntity(1)) ENGINE_LOG_WARN("Scene is full; remove an entity before adding another.");
                    } else if (mouseX >= 176.0f && mouseX <= 265.0f) {
                        if (!addEntity(2)) ENGINE_LOG_WARN("Scene is full; remove an entity before adding another.");
                    } else if (mouseX >= 271.0f && mouseX <= 348.0f) {
                        if (!addEntity(3)) ENGINE_LOG_WARN("Scene is full; remove an entity before adding another.");
                    } else if (mouseX >= 354.0f && mouseX <= 430.0f) {
                        if (!addEntity(4)) ENGINE_LOG_WARN("Scene is full; remove an entity before adding another.");
                    } else if (mouseX >= 436.0f && mouseX <= 525.0f) {
                        debugDraw = !debugDraw;
                        ENGINE_LOG_INFO("Editor: Toggled Debug Draw (F5) -> {}", debugDraw ? "ON" : "OFF");
                    } else if (mouseX >= 531.0f && mouseX <= 605.0f) {
                        deleteSelectedEntity();
                    }
                }
                // 5. Outliner / Hierarchy Selection (X < 270, Y >= 74)
                else if (mouseX >= 5.0f && mouseX <= 265.0f && mouseY >= 74.0f && mouseY < static_cast<float>(winDesc.height) - 200.0f) {
                    int clickedRow = static_cast<int>((mouseY - 108.0f) / 30.0f);
                    if (clickedRow >= 0 && clickedRow < static_cast<int>(sceneEntities.size())) {
                        selectedEntity = clickedRow;
                        draggingEntity = -1;
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
                        const Ray3 ray = MakeViewportRay(mouseX, mouseY, winDesc.width, winDesc.height,
                            camPosX, camPosY, camPosZ, vpCamYaw, vpCamPitch);
                        selectedEntity = PickEntity(ray, sceneEntities);
                        if (selectedEntity >= 0) {
                            draggingEntity = selectedEntity;
                            dragUndoSnapshot = sceneEntities;
                            dragUndoRecorded = false;
                            ENGINE_LOG_INFO("Viewport selected '{}' ({} transform mode).", sceneEntities[selectedEntity].name,
                                gizmoMode == 0 ? "select" : gizmoMode == 1 ? "move" : gizmoMode == 2 ? "rotate" : "scale");
                        } else {
                            draggingEntity = -1;
                        }
                    }
                }
                // 7. Details / Inspector [+] [-] and [+ ADD COMPONENT]
                else if (mouseX >= static_cast<float>(winDesc.width) - 330.0f && mouseY >= 74.0f && mouseY < static_cast<float>(winDesc.height) - 200.0f) {
                    float rightDockX = static_cast<float>(winDesc.width) - 330.0f;
                    float relY = mouseY - (74.0f + 36.0f);

                    if (selectedEntity >= 0 && selectedEntity < static_cast<int>(sceneEntities.size())) {
                        auto& ent = sceneEntities[selectedEntity];
                        const int transformRow = static_cast<int>((mouseY - 140.0f) / 28.0f);
                        const float rowY = 142.0f + static_cast<float>(transformRow) * 28.0f;
                        const bool rowHit = transformRow >= 0 && transformRow <= 4 && mouseY >= rowY - 2.0f && mouseY <= rowY + 20.0f;
                        if (rowHit) {
                            const bool decrement = mouseX >= rightDockX + 107.0f && mouseX <= rightDockX + 132.0f;
                            const bool increment = mouseX >= rightDockX + 275.0f && mouseX <= rightDockX + 300.0f;
                            if (decrement || increment) {
                                pushUndoSnapshot();
                                const float amount = increment ? 0.25f : -0.25f;
                                if (transformRow == 0) ent.posX += amount;
                                else if (transformRow == 1) ent.posY += amount;
                                else if (transformRow == 2) ent.posZ += amount;
                                else if (transformRow == 3) ent.rotY += amount * 0.1f;
                                else {
                                    const float scale = std::clamp(ent.scaleX + amount * 0.1f, 0.1f, 20.0f);
                                    ent.scaleX = scale;
                                    ent.scaleY = scale;
                                    ent.scaleZ = scale;
                                }
                            }
                        }
                        // [+ ADD COMPONENT] BUTTON
                        else if (relY >= 340.0f && relY <= 370.0f) {
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
                    } else if (bottomTab == 0 && mouseY >= bDockY + 38.0f && mouseY <= bDockY + 138.0f) {
                        for (int primitive = 0; primitive < 5; ++primitive) {
                            const float cardMinX = 200.0f + static_cast<float>(primitive) * 115.0f;
                            if (mouseX >= cardMinX && mouseX <= cardMinX + 100.0f) {
                                if (!addEntity(primitive)) {
                                    ENGINE_LOG_WARN("Scene is full; remove an entity before adding another.");
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }

        // Physics Simulation in Play Mode
        if (simState == 1) {
            for (auto& ent : sceneEntities) {
                if ((ent.componentMask & ComponentRigidBody) != 0u && ent.type <= 2) {
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
            cmdList->ClearRenderTarget(backBuffer, 0.08f, 0.09f, 0.12f, 1.0f);

            // 64 root constants carry view state, scene transforms, and inspector values.
            float pushConstants[64]{};
            pushConstants[0] = static_cast<float>(winDesc.width);
            pushConstants[1] = static_cast<float>(winDesc.height);
            pushConstants[2] = vpCamYaw;
            pushConstants[3] = vpCamPitch;
            pushConstants[4] = camPosX;
            pushConstants[5] = camPosY;
            pushConstants[6] = camPosZ;
            u32 packedState = static_cast<u32>(simState) & 0x3u;
            packedState |= (static_cast<u32>(selectedEntity >= 0 ? selectedEntity + 1 : 0) & 0xFu) << 2u;
            packedState |= (static_cast<u32>(activeMenu) & 0x7u) << 6u;
            packedState |= (static_cast<u32>(bottomTab) & 0x1u) << 9u;
            packedState |= (static_cast<u32>(activeModal) & 0x3u) << 10u;
            packedState |= (static_cast<u32>(sceneEntities.size()) & 0xFu) << 12u;
            packedState |= (static_cast<u32>(gizmoMode) & 0x3u) << 16u;
            packedState |= (debugDraw ? 1u : 0u) << 18u;
            if (selectedEntity >= 0 && selectedEntity < static_cast<int>(sceneEntities.size())) {
                packedState |= (sceneEntities[selectedEntity].componentMask & 0x1Fu) << 19u;
            }
            pushConstants[7] = static_cast<float>(packedState);

            for (size_t i = 0; i < 8 && i < sceneEntities.size(); ++i) {
                pushConstants[8 + i * 4 + 0] = sceneEntities[i].posX;
                pushConstants[8 + i * 4 + 1] = sceneEntities[i].posY;
                pushConstants[8 + i * 4 + 2] = sceneEntities[i].posZ;
                pushConstants[8 + i * 4 + 3] = static_cast<float>(sceneEntities[i].type);
                pushConstants[40 + i * 2 + 0] = sceneEntities[i].rotY;
                pushConstants[40 + i * 2 + 1] = sceneEntities[i].scaleX;
            }

            pushConstants[56] = Platform::Input::Get().GetMouseX();
            pushConstants[57] = Platform::Input::Get().GetMouseY();
            if (selectedEntity >= 0 && selectedEntity < static_cast<int>(sceneEntities.size())) {
                const auto& selected = sceneEntities[selectedEntity];
                pushConstants[58] = selected.posX;
                pushConstants[59] = selected.posY;
                pushConstants[60] = selected.posZ;
                pushConstants[61] = selected.rotY * (180.0f / std::numbers::pi_v<float>);
                pushConstants[62] = selected.scaleX;
            }
            pushConstants[63] = sceneDirty ? 1.0f : 0.0f;

            cmdList->SetRoot32BitConstants(0, 64, pushConstants, 0);
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
                "Arcglass Engine Editor | {:.1f} FPS ({:.2f} ms) | Entities: {} | Mode: {} | Debug: {}",
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
