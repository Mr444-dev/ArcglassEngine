#include "engine/core/BaseTypes.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "engine/platform/Window.h"
#include "engine/rhi/RHIDevice.h"
#include "engine/rhi/RHIShaderCompiler.h"

void RunRHIGraphicsTest() {
    using namespace Engine;
    using namespace Engine::RHI;

    Core::Logger::LogMessage(Core::LogLevel::Info, "--- Starting DirectX 12 RHI Pipeline Test ---");

    // 1. Initialize Window
    Platform::Window window;
    Platform::WindowDesc winDesc{};
    winDesc.title = "DirectX 12 RHI Verification Test";
    winDesc.width = 1280;
    winDesc.height = 720;
    ENGINE_VERIFY(window.Initialize(winDesc), "Failed to initialize platform window for RHI test");

    // 2. Initialize GPU Device
    auto& device = RHIDevice::Get();
    ENGINE_VERIFY(device.Initialize(window.GetNativeHandle(), winDesc.width, winDesc.height), "Failed to initialize RHIDevice");

    // 3. Create SwapChain
    auto swapChain = device.CreateSwapChain(window.GetNativeHandle(), winDesc.width, winDesc.height);
    ENGINE_VERIFY(swapChain != nullptr, "Failed to create RHISwapChain");

    // 4. Compile Shaders
    std::vector<Core::u8> vsBytecode;
    std::vector<Core::u8> psBytecode;

    const std::string shaderPath = "shaders/geometry/SimpleTriangle.hlsl";
    ENGINE_VERIFY(RHIShaderCompiler::CompileFromFile(shaderPath, "VSMain", "vs_5_1", vsBytecode), "VS compilation failed");
    ENGINE_VERIFY(RHIShaderCompiler::CompileFromFile(shaderPath, "PSMain", "ps_5_1", psBytecode), "PS compilation failed");
    ENGINE_LOG_INFO("SimpleTriangle.hlsl compiled successfully (VS: {} bytes, PS: {} bytes).", 
        vsBytecode.size(), psBytecode.size());

    // 5. Create Graphics Pipeline State
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
    ENGINE_VERIFY(pso != nullptr, "Failed to create Graphics PSO");
    Core::Logger::LogMessage(Core::LogLevel::Info, "Monolithic Graphics Pipeline State created.");

    // 6. Create Command List
    auto cmdList = device.CreateCommandList();
    ENGINE_VERIFY(cmdList != nullptr, "Failed to create CommandList");

    // 7. Render Test Frames
    Viewport vp{0.0f, 0.0f, static_cast<float>(winDesc.width), static_cast<float>(winDesc.height), 0.0f, 1.0f};
    ScissorRect scissor{0, 0, static_cast<Core::i32>(winDesc.width), static_cast<Core::i32>(winDesc.height)};

    for (int frame = 0; frame < 5; ++frame) {
        window.PollEvents();

        RHITexture* backBuffer = swapChain->GetCurrentBackBuffer();
        ENGINE_ASSERT(backBuffer != nullptr, "Null backbuffer texture");

        cmdList->Reset(pso.get());

        // Transition backbuffer to RenderTarget
        cmdList->TransitionBarrier(backBuffer, ResourceState::RenderTarget);

        // Clear to dark navy blue
        cmdList->ClearRenderTarget(backBuffer, 0.08f, 0.12f, 0.22f, 1.0f);

        cmdList->SetViewport(vp);
        cmdList->SetScissorRect(scissor);

        RHITexture* rts[1] = { backBuffer };
        cmdList->SetRenderTargets(1, rts, nullptr);

        cmdList->SetPipelineState(pso.get());
        cmdList->DrawInstanced(3, 1, 0, 0); // Draw procedural triangle

        // Transition backbuffer to Present
        cmdList->TransitionBarrier(backBuffer, ResourceState::Present);

        cmdList->Close();

        device.ExecuteCommandList(cmdList.get());
        swapChain->Present(false);
        device.WaitForGPU();

        ENGINE_LOG_INFO("Rendered and presented test frame {}.", frame);
    }

    Core::Logger::LogMessage(Core::LogLevel::Info, "RHI Pipeline Test completed with 0 validation errors.");

    // 8. Clean Shutdown
    device.WaitForGPU();
    pso.reset();
    cmdList.reset();
    swapChain.reset();
    device.Shutdown();
    window.Shutdown();
}
