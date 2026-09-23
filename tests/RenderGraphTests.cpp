#include "engine/core/BaseTypes.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "engine/renderer/Camera.h"
#include "engine/renderer/RenderGraph.h"

void TestCameraAndFrustum() {
    using namespace Engine::Renderer;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Camera & Frustum Culling ---");

    Camera camera;
    camera.SetPosition({0.0f, 0.0f, -5.0f});
    camera.SetTarget({0.0f, 0.0f, 0.0f});
    camera.SetLens(0.785398f, 16.0f / 9.0f, 0.1f, 100.0f);
    camera.UpdateMatrices();

    ENGINE_LOG_INFO("{}", camera.GetDebugString());

    const auto& frustum = camera.GetFrustum();

    // 1. AABB directly in front of the camera (should be VISIBLE)
    AABB visibleBox;
    visibleBox.min = {-1.0f, -1.0f, 0.0f};
    visibleBox.max = { 1.0f,  1.0f, 2.0f};
    bool isVisible = frustum.IntersectsAABB(visibleBox);
    ENGINE_ASSERT(isVisible, "Error: Visible AABB failed frustum intersection test!");
    ENGINE_LOG_INFO("AABB in front of camera correctly identified as VISIBLE.");

    // 2. AABB behind the camera (should be CULLED)
    AABB behindBox;
    behindBox.min = {-1.0f, -1.0f, -50.0f};
    behindBox.max = { 1.0f,  1.0f, -40.0f};
    bool isBehind = frustum.IntersectsAABB(behindBox);
    ENGINE_ASSERT(!isBehind, "Error: AABB behind camera was not culled!");
    ENGINE_LOG_INFO("AABB behind camera correctly identified as CULLED.");

    // 3. AABB far outside the view (should be CULLED)
    AABB outsideBox;
    outsideBox.min = {500.0f, 500.0f, 10.0f};
    outsideBox.max = {510.0f, 510.0f, 20.0f};
    bool isOutside = frustum.IntersectsAABB(outsideBox);
    ENGINE_ASSERT(!isOutside, "Error: AABB far outside frustum was not culled!");
    ENGINE_LOG_INFO("AABB outside view frustum correctly identified as CULLED.");

    ENGINE_LOG_INFO("Camera and Frustum tests passed all checks.");
}

void TestRenderGraphTopologicalSortAndCulling() {
    using namespace Engine::Renderer;
    using namespace Engine::RHI;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing RenderGraph Topological Sort & Dead-Code Culling ---");

    RenderGraph graph;

    // 1. Declare resources
    RGTextureDesc depthDesc{1280, 720, Format::D32_FLOAT, TextureUsage::DepthStencil, "DepthBuffer"};
    RGTextureDesc hdrDesc{1280, 720, Format::RGBA16_FLOAT, TextureUsage::RenderTarget | TextureUsage::ShaderResource, "HDRColor"};
    RGTextureDesc unreferencedDesc{1280, 720, Format::RGBA8_UNORM, TextureUsage::RenderTarget, "UnreferencedTexture"};

    RGTextureHandle depthTex = graph.DeclareTexture("Depth", depthDesc);
    RGTextureHandle hdrTex = graph.DeclareTexture("HDRColor", hdrDesc);
    RGTextureHandle unrefTex = graph.DeclareTexture("Unreferenced", unreferencedDesc);

    // Mock an imported SwapChain backbuffer texture (has refCount > 0 by definition)
    RGTextureDesc swapDesc{1280, 720, Format::RGBA8_UNORM, TextureUsage::RenderTarget, "SwapChainBackBuffer"};
    RGTextureHandle backBuffer = graph.DeclareTexture("BackBuffer", swapDesc);
    graph.MarkOutput(backBuffer);

    // 2. Add Pass 1: Depth Prepass (writes Depth)
    graph.AddPass(
        "DepthPrepass",
        [&](RGPassBuilder& builder) {
            builder.WriteDepthStencil(depthTex, ResourceState::DepthWrite);
        },
        [](RHICommandList&, const RenderGraph&) {}
    );

    // 3. Add Pass 2: Dead / Unused Pass (writes to unreferenced texture - should be CULLED)
    graph.AddPass(
        "DeadPass",
        [&](RGPassBuilder& builder) {
            builder.WriteTexture(unrefTex, ResourceState::RenderTarget);
        },
        [](RHICommandList&, const RenderGraph&) {}
    );

    // 4. Add Pass 3: Lighting Pass (reads Depth, writes HDRColor)
    graph.AddPass(
        "LightingPass",
        [&](RGPassBuilder& builder) {
            builder.ReadTexture(depthTex, ResourceState::DepthRead);
            builder.WriteTexture(hdrTex, ResourceState::RenderTarget);
        },
        [](RHICommandList&, const RenderGraph&) {}
    );

    // 5. Add Pass 4: PostProcess Pass (reads HDRColor, writes BackBuffer)
    graph.AddPass(
        "PostProcessPass",
        [&](RGPassBuilder& builder) {
            builder.ReadTexture(hdrTex, ResourceState::ShaderResource);
            builder.WriteTexture(backBuffer, ResourceState::RenderTarget);
        },
        [](RHICommandList&, const RenderGraph&) {}
    );

    // 6. Compile graph
    ENGINE_VERIFY(graph.Compile(), "RenderGraph failed to compile DAG");

    // 7. Verify pass count and culling
    // Total passes declared: 4. Pass "DeadPass" should be culled!
    ENGINE_ASSERT(graph.GetPassCount() == 4, "Total pass count mismatch");
    ENGINE_ASSERT(graph.GetCompiledPassCount() == 3, "Dead pass was not properly culled! Expected 3 active passes.");

    ENGINE_LOG_INFO("RenderGraph successfully scheduled 3 active passes and culled 1 dead pass.");
}

void TestRenderGraphCycleDetection() {
    using namespace Engine::Renderer;
    using namespace Engine::RHI;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing RenderGraph Circular Dependency Guardrail ---");

    RenderGraph cycleGraph;

    RGTextureDesc tDesc{1280, 720, Format::RGBA8_UNORM, TextureUsage::RenderTarget | TextureUsage::ShaderResource, "CycleTex"};
    RGTextureHandle texA = cycleGraph.DeclareTexture("TexA", tDesc);
    RGTextureHandle texB = cycleGraph.DeclareTexture("TexB", tDesc);

    // Pass A writes TexA and reads TexB
    cycleGraph.AddPass(
        "PassA",
        [&](RGPassBuilder& builder) {
            builder.WriteTexture(texA, ResourceState::RenderTarget);
            builder.ReadTexture(texB, ResourceState::ShaderResource);
        },
        [](RHICommandList&, const RenderGraph&) {}
    );

    // Pass B writes TexB and reads TexA (creates circular dependency: PassA -> PassB -> PassA)
    cycleGraph.AddPass(
        "PassB",
        [&](RGPassBuilder& builder) {
            builder.WriteTexture(texB, ResourceState::RenderTarget);
            builder.ReadTexture(texA, ResourceState::ShaderResource);
        },
        [](RHICommandList&, const RenderGraph&) {}
    );

    // Compiling this MUST detect the cycle, report it clearly, and return false
    bool compileResult = cycleGraph.Compile();
    ENGINE_ASSERT(!compileResult, "Error: RenderGraph failed to catch circular dependency!");

    ENGINE_LOG_INFO("RenderGraph circular dependency guardrail correctly detected and prevented cyclic execution.");
}

void RunRenderGraphUnitTests() {
    TestCameraAndFrustum();
    TestRenderGraphTopologicalSortAndCulling();
    TestRenderGraphCycleDetection();
}
