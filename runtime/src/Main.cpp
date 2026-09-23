#include "engine/core/BaseTypes.h"
#include "engine/core/Log.h"
#include "engine/core/Profiler.h"
#include "engine/core/Allocators.h"
#include "engine/core/JobSystem.h"
#include "engine/platform/Platform.h"
#include "engine/platform/Window.h"
#include "engine/platform/Input.h"
#include "engine/rhi/RHI.h"
#include "engine/rhi/RHIDevice.h"
#include "engine/rhi/RHIShaderCompiler.h"
#include "engine/renderer/Camera.h"
#include "engine/renderer/RenderGraph.h"
#include "engine/renderer/MaterialSystem.h"
#include "engine/renderer/DebugRenderer.h"
#include "engine/renderer/passes/DepthPrepass.h"
#include "engine/renderer/passes/LightingPass.h"
#include "engine/renderer/passes/PostProcessPass.h"
#include "engine/renderer/passes/VolumetricFogPass.h"
#include "engine/renderer/VoxelGlobalIllumination.h"
#include "engine/renderer/NeuralRadianceCache.h"
#include "engine/renderer/WorldStreaming.h"
#include <chrono>
#include <cmath>
#include <algorithm>
#include <string>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    using namespace Engine;
    using namespace Engine::RHI;
    using namespace Engine::Renderer;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "=========================================================");
    Logger::LogMessage(LogLevel::Info, "   Modern 3D Game Engine (Standard 2026) - Tech Demo   ");
    Logger::LogMessage(LogLevel::Info, "=========================================================");

    // 1. Core Systems Startup
    Profiler::Initialize();
    JobSystem::Get().Initialize();

    // 2. Platform & Windowing
    Platform::Initialize();
    Platform::Window window;
    Platform::WindowDesc winDesc{};
    winDesc.title = "Modern 3D Game Engine (2026) | Initializing...";
    winDesc.width = 1280;
    winDesc.height = 720;
    ENGINE_VERIFY(window.Initialize(winDesc), "Failed to initialize game window");

    // 3. Low-Level Graphics RHI
    auto& device = RHIDevice::Get();
    ENGINE_VERIFY(device.Initialize(window.GetNativeHandle(), winDesc.width, winDesc.height), "Failed to initialize RHI GPU device");

    auto swapChain = device.CreateSwapChain(window.GetNativeHandle(), winDesc.width, winDesc.height);
    ENGINE_VERIFY(swapChain != nullptr, "Failed to create SwapChain");

    auto cmdList = device.CreateCommandList();
    ENGINE_VERIFY(cmdList != nullptr, "Failed to create CommandList");

    // 4. Player & Gameplay Mechanics (Crystal Seeker 3D)
    Vec3 playerPos{0.0f, 0.0f, 0.0f};
    Vec3 playerVel{0.0f, 0.0f, 0.0f};
    float playerYaw = 0.0f;
    Core::u32 crystalMask = 0;
    Core::u32 score = 0;

    const Vec3 crystalLocations[5] = {
        Vec3{  0.0f, 1.2f,  10.0f},
        Vec3{ 12.0f, 1.2f,   4.0f},
        Vec3{-10.0f, 1.2f,  -6.0f},
        Vec3{ 14.0f, 1.2f,  16.0f},
        Vec3{-14.0f, 1.2f,  10.0f}
    };

    Camera camera;
    Vec3 camPos{0.0f, 3.0f, -6.0f};
    float camYaw = 0.0f;
    float camPitch = -0.12f;

    camera.SetPosition(camPos);
    camera.SetTarget({0.0f, 0.0f, 0.0f});
    camera.SetLens(0.785398f, static_cast<float>(winDesc.width) / static_cast<float>(winDesc.height), 0.1f, 1000.0f);
    camera.UpdateMatrices();

    MaterialSystem materialSystem;
    ENGINE_VERIFY(materialSystem.Initialize(), "Failed to initialize MaterialSystem");

    DebugRenderer debugRenderer;
    ENGINE_VERIFY(debugRenderer.Initialize(), "Failed to initialize DebugRenderer");

    // Compile Interactive 3D Runtime Scene Shaders
    std::vector<u8> vsBytecode;
    std::vector<u8> psBytecode;
    bool vsOk = RHIShaderCompiler::CompileFromFile("shaders/geometry/RuntimeScene.hlsl", "VSMain", "vs_5_1", vsBytecode);
    bool psOk = RHIShaderCompiler::CompileFromFile("shaders/geometry/RuntimeScene.hlsl", "PSMain", "ps_5_1", psBytecode);
    ENGINE_VERIFY(vsOk && psOk, "Failed to compile Runtime Scene shaders");

    GraphicsPipelineDesc scenePsoDesc{};
    scenePsoDesc.vertexShaderBytecode = vsBytecode.data();
    scenePsoDesc.vertexShaderSize = vsBytecode.size();
    scenePsoDesc.pixelShaderBytecode = psBytecode.data();
    scenePsoDesc.pixelShaderSize = psBytecode.size();
    scenePsoDesc.renderTargetCount = 1;
    scenePsoDesc.renderTargetFormats[0] = Format::RGBA8_UNORM;
    scenePsoDesc.depthStencilFormat = Format::Unknown;
    scenePsoDesc.depthEnable = false;
    scenePsoDesc.depthWrite = false;
    scenePsoDesc.cullBackFace = false;

    auto scenePso = device.CreateGraphicsPipelineState(scenePsoDesc);
    ENGINE_VERIFY(scenePso != nullptr, "Failed to create Runtime Scene Graphics PSO");

    // Next-Gen Renderer Subsystem Instances
    NeuralRadianceCache nrc;
    TerrainClipmap terrainClipmap(1.0f, 32.0f);
    VirtualTextureManager svtManager;

    FrameAllocator frameAlloc(4 * MB);
    RenderGraph renderGraph;

    ENGINE_LOG_INFO("Engine initialized. Controls: WASD to move, Space/Ctrl for Up/Down, Hold RMB to look, ESC to exit.");

    auto prevTime = std::chrono::high_resolution_clock::now();
    float fpsTimer = 0.0f;
    Core::u32 fpsFrameCount = 0;
    Core::u64 frameIndex = 0;

    while (window.PollEvents()) {
        Platform::Input::Get().BeginFrame();
        frameAlloc.BeginFrame();

        if (Platform::Input::Get().IsKeyDown(Platform::KeyCode::Escape)) {
            break;
        }

        // Calculate frame delta time
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - prevTime).count();
        prevTime = currentTime;
        deltaTime = std::clamp(deltaTime, 0.0001f, 0.1f);

        // --- 3D Character Physics & Controls ---
        float turnSpeed = 2.4f; // rad/s
        if (Platform::Input::Get().IsKeyDown(Platform::KeyCode::A)) playerYaw -= turnSpeed * deltaTime;
        if (Platform::Input::Get().IsKeyDown(Platform::KeyCode::D)) playerYaw += turnSpeed * deltaTime;

        const Vec3 pForward{ std::sin(playerYaw), 0.0f, std::cos(playerYaw) };
        float driveSpeed = 0.0f;
        float maxSpeed = 14.0f; // m/s
        if (Platform::Input::Get().IsKeyDown(Platform::KeyCode::LeftShift)) maxSpeed = 26.0f; // Nitro

        if (Platform::Input::Get().IsKeyDown(Platform::KeyCode::W)) driveSpeed = maxSpeed;
        if (Platform::Input::Get().IsKeyDown(Platform::KeyCode::S)) driveSpeed = -maxSpeed * 0.5f;

        playerPos = playerPos + pForward * (driveSpeed * deltaTime);

        // Physics Jump & Gravity
        if (Platform::Input::Get().WasKeyPressed(Platform::KeyCode::Space) && playerPos.y <= 0.01f) {
            playerVel.y = 8.5f;
        }
        playerVel.y -= 22.0f * deltaTime;
        playerPos.y += playerVel.y * deltaTime;
        if (playerPos.y < 0.0f) {
            playerPos.y = 0.0f;
            playerVel.y = 0.0f;
        }

        // Restart with R
        if (Platform::Input::Get().WasKeyPressed(Platform::KeyCode::R)) {
            crystalMask = 0;
            score = 0;
            playerPos = Vec3{0, 0, 0};
            ENGINE_LOG_INFO("Crystal Seeker 3D restarted!");
        }

        // Crystal Collision Detection (1.8m pickup radius)
        for (int i = 0; i < 5; ++i) {
            if (!(crystalMask & (1 << i))) {
                Vec3 diff = playerPos - crystalLocations[i];
                float distSq = diff.x * diff.x + diff.z * diff.z;
                if (distSq < 3.24f) { // 1.8m^2
                    crystalMask |= (1 << i);
                    score += 100;
                    ENGINE_LOG_INFO("Collected Crystal {}! Total Score: {}", i + 1, score);
                }
            }
        }

        // Camera Smooth Chase Behind Player
        camYaw = playerYaw;
        camPitch = -0.15f;
        if (Platform::Input::Get().IsMouseButtonDown(Platform::MouseButton::Right)) {
            camYaw += Platform::Input::Get().GetMouseDeltaX() * 0.005f;
            camPitch -= Platform::Input::Get().GetMouseDeltaY() * 0.005f;
        }
        const Vec3 camForward{
            std::sin(camYaw) * std::cos(camPitch),
            std::sin(camPitch),
            std::cos(camYaw) * std::cos(camPitch)
        };
        camPos = playerPos - camForward * 6.5f + Vec3{0.0f, 2.8f, 0.0f};

        camera.SetPosition(camPos);
        camera.SetTarget(playerPos + Vec3{0.0f, 1.0f, 0.0f});
        camera.UpdateMatrices();

        // Update terrain clipmap position snapped to grid spacing
        terrainClipmap.UpdateCameraPosition(camPos);

        // --- Telemetry & Window Title ---
        fpsTimer += deltaTime;
        fpsFrameCount++;
        if (fpsTimer >= 0.5f) {
            const float fps = static_cast<float>(fpsFrameCount) / fpsTimer;
            const float ms = 1000.0f / (fps > 0.0f ? fps : 1.0f);

            char titleBuffer[256];
            snprintf(titleBuffer, sizeof(titleBuffer),
                "Modern 3D Game Engine (Standard 2026) | %.0f FPS (%.2f ms) | Cam: [%.1f, %.1f, %.1f] | Active Passes: %zu",
                fps, ms, camPos.x, camPos.y, camPos.z, renderGraph.GetCompiledPassCount());
            window.SetTitle(titleBuffer);

            fpsTimer = 0.0f;
            fpsFrameCount = 0;
        }

        // --- Debug Scene Visualization ---
        debugRenderer.Clear();

        // Ground reference grid (gray lines)
        const float gridColor[4] = {0.25f, 0.25f, 0.30f, 1.0f};
        for (int i = -15; i <= 15; i += 3) {
            const float fi = static_cast<float>(i);
            debugRenderer.AddLine({fi, 0.0f, -15.0f}, {fi, 0.0f, 15.0f}, gridColor);
            debugRenderer.AddLine({-15.0f, 0.0f, fi}, {15.0f, 0.0f, fi}, gridColor);
        }

        // World Origin Coordinate Axes (Red = X, Green = Y, Blue = Z)
        const float red[4] = {1.0f, 0.0f, 0.0f, 1.0f};
        const float green[4] = {0.0f, 1.0f, 0.0f, 1.0f};
        const float blue[4] = {0.0f, 0.0f, 1.0f, 1.0f};
        debugRenderer.AddLine({0.0f, 0.0f, 0.0f}, {3.0f, 0.0f, 0.0f}, red);
        debugRenderer.AddLine({0.0f, 0.0f, 0.0f}, {0.0f, 3.0f, 0.0f}, green);
        debugRenderer.AddLine({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 3.0f}, blue);

        // Center Showcase Bounding Box (Cyan)
        const float cyan[4] = {0.0f, 1.0f, 1.0f, 1.0f};
        debugRenderer.AddAABB({{-1.5f, 0.0f, -1.5f}, {1.5f, 3.0f, 1.5f}}, cyan);

        // Procedural Terrain Clipmap LOD 0 Ring Center Indicator (Yellow)
        const float yellow[4] = {1.0f, 0.9f, 0.1f, 1.0f};
        const auto& levels = terrainClipmap.GetLevels();
        const Vec3 ring0Center{levels[0].centerOffset.x, 0.1f, levels[0].centerOffset.y};
        debugRenderer.AddSphere(ring0Center, 0.5f, yellow, 8);

        // --- RenderGraph Frame Setup & Execution ---
        RHITexture* backBuffer = swapChain->GetCurrentBackBuffer();
        if (!backBuffer) continue;

        renderGraph.Reset();

        // 1. External Output Sink (BackBuffer)
        RGTextureHandle backBufferHandle = renderGraph.ImportTexture("SwapChainBackBuffer", backBuffer, ResourceState::Common);
        renderGraph.MarkOutput(backBufferHandle);

        // 2. Declare Deferred & Next-Gen G-Buffer Targets
        RGTextureDesc depthDesc{winDesc.width, winDesc.height, Format::D32_FLOAT, TextureUsage::DepthStencil, "GBufferDepth"};
        RGTextureHandle depthBuffer = renderGraph.DeclareTexture("GBufferDepth", depthDesc);

        RGTextureDesc normalDesc{winDesc.width, winDesc.height, Format::RGBA16_FLOAT, TextureUsage::RenderTarget | TextureUsage::ShaderResource, "GBufferNormal"};
        RGTextureHandle normalBuffer = renderGraph.DeclareTexture("GBufferNormal", normalDesc);

        RGTextureDesc hdrDesc{winDesc.width, winDesc.height, Format::RGBA16_FLOAT, TextureUsage::RenderTarget | TextureUsage::ShaderResource, "HDRSceneColor"};
        RGTextureHandle hdrColor = renderGraph.DeclareTexture("HDRSceneColor", hdrDesc);

        // 3. Volumetric Fog Froxel Grid Target
        RGTextureDesc fogDesc{160, 90, Format::RGBA16_FLOAT, TextureUsage::RenderTarget | TextureUsage::ShaderResource, "VolumetricFogVolume"};
        RGTextureHandle fogVolume = renderGraph.DeclareTexture("VolumetricFogVolume", fogDesc);

        // 4. Voxel Global Illumination 3D Radiance Target
        RGTextureDesc voxel3DDesc{128, 128, Format::RGBA16_FLOAT, TextureUsage::RenderTarget | TextureUsage::ShaderResource, "VoxelRadiance3D"};
        RGTextureHandle voxelRadiance3D = renderGraph.DeclareTexture("VoxelRadiance3D", voxel3DDesc);

        RGTextureDesc indirectDiffuseDesc{winDesc.width, winDesc.height, Format::RGBA16_FLOAT, TextureUsage::RenderTarget | TextureUsage::ShaderResource, "VoxelIndirectDiffuse"};
        RGTextureHandle indirectDiffuse = renderGraph.DeclareTexture("VoxelIndirectDiffuse", indirectDiffuseDesc);

        // 5. Neural Radiance Cache Output Target
        RGTextureDesc nrcDesc{winDesc.width, winDesc.height, Format::RGBA16_FLOAT, TextureUsage::RenderTarget | TextureUsage::ShaderResource, "NRCIndirectLight"};
        RGTextureHandle nrcIndirect = renderGraph.DeclareTexture("NRCIndirectLight", nrcDesc);

        // 6. Terrain Clipmap Heightmap & Scene Surface Output Target
        RGTextureDesc heightDesc{1024, 1024, Format::R32_FLOAT, TextureUsage::ShaderResource, "TerrainHeightmap"};
        RGTextureHandle terrainHeight = renderGraph.DeclareTexture("TerrainHeightmap", heightDesc);

        RGTextureDesc terrainOutDesc{winDesc.width, winDesc.height, Format::RGBA8_UNORM, TextureUsage::RenderTarget | TextureUsage::ShaderResource, "TerrainSceneColor"};
        RGTextureHandle terrainOutput = renderGraph.DeclareTexture("TerrainSceneColor", terrainOutDesc);

        // --- Schedule RenderGraph Passes ---
        DepthPrepass::AddToGraph(renderGraph, depthBuffer);
        VolumetricFogPass::AddToGraph(renderGraph, depthBuffer, fogVolume);

        VoxelGIConfig voxelConfig{};
        voxelConfig.volumeResolution = 128;
        voxelConfig.volumeWorldExtent = 64.0f;
        VoxelGlobalIllumination::AddToGraph(renderGraph, normalBuffer, depthBuffer, voxelRadiance3D, indirectDiffuse, voxelConfig);

        nrc.AddToGraph(renderGraph, normalBuffer, depthBuffer, nrcIndirect);
        VirtualTextureManager::AddToGraph(renderGraph, depthBuffer, terrainHeight, terrainOutput);

        LightingPass::AddToGraph(renderGraph, depthBuffer, hdrColor);
        PostProcessPass::AddToGraph(renderGraph, hdrColor, backBufferHandle);

        // Compile Graph (DAG topological sort, barrier generation, pass scheduling)
        if (!renderGraph.IsCompiled()) {
            ENGINE_VERIFY(renderGraph.Compile(), "RenderGraph compilation failed");
        }

        // Record and Execute Command List on GPU
        cmdList->Reset(scenePso.get());
        renderGraph.Execute(*cmdList);
        debugRenderer.Flush(*cmdList, camera);

        // Render full interactive 3D perspective world to back buffer
        cmdList->TransitionBarrier(backBuffer, ResourceState::RenderTarget);
        Viewport vp{0.0f, 0.0f, static_cast<float>(winDesc.width), static_cast<float>(winDesc.height), 0.0f, 1.0f};
        ScissorRect scissor{0, 0, static_cast<i32>(winDesc.width), static_cast<i32>(winDesc.height)};
        cmdList->SetViewport(vp);
        cmdList->SetScissorRect(scissor);

        RHITexture* rts[1] = { backBuffer };
        cmdList->SetRenderTargets(1, rts, nullptr);
        cmdList->SetPipelineState(scenePso.get());

        static float totalRuntimeSeconds = 0.0f;
        totalRuntimeSeconds += deltaTime;
        float pushConstants[16] = {
            totalRuntimeSeconds,
            static_cast<float>(winDesc.width),
            static_cast<float>(winDesc.height),
            camYaw,
            camPitch,
            camPos.x,
            camPos.y,
            camPos.z,
            playerPos.x,
            playerPos.y,
            playerPos.z,
            playerYaw,
            static_cast<float>(crystalMask),
            static_cast<float>(score),
            driveSpeed,
            (crystalMask == 31) ? 1.0f : 0.0f
        };
        cmdList->SetRoot32BitConstants(0, 16, pushConstants, 0);
        cmdList->DrawInstanced(3, 1, 0, 0);

        cmdList->TransitionBarrier(backBuffer, ResourceState::Present);
        cmdList->Close();

        device.ExecuteCommandList(cmdList.get());
        swapChain->Present(true); // V-Sync enabled
        device.WaitForGPU();

        frameIndex++;
    }

    // 6. Clean Shutdown in Reverse DAG Order
    ENGINE_LOG_INFO("Shutting down GameRuntime cleanly...");
    device.WaitForGPU();

    debugRenderer.Shutdown();
    materialSystem.Shutdown();
    renderGraph.Reset();

    cmdList.reset();
    swapChain.reset();
    device.Shutdown();
    window.Shutdown();
    Platform::Shutdown();

    JobSystem::Get().Shutdown();
    Profiler::Shutdown();

    ENGINE_LOG_INFO("GameRuntime closed successfully (Total frames rendered: {}).", frameIndex);
    return 0;
}
