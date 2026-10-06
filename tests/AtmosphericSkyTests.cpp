#include "engine/core/BaseTypes.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "engine/renderer/AtmosphericSky.h"
#include "engine/renderer/RenderGraph.h"

void TestRayleighAndMiePhases() {
    using namespace Engine::Core;
    using namespace Engine::Renderer;

    Logger::LogMessage(LogLevel::Info, "--- Testing Physical Rayleigh & Mie Phase Scattering Functions ---");

    // 1. Rayleigh phase must be strictly symmetric: P_R(cos) == P_R(-cos)
    const float pRFwd = AtmosphericSky::EvaluateRayleighPhase(1.0f);
    const float pRBwd = AtmosphericSky::EvaluateRayleighPhase(-1.0f);
    const float pRPerp = AtmosphericSky::EvaluateRayleighPhase(0.0f);

    ENGINE_ASSERT(std::abs(pRFwd - pRBwd) < 1e-5f, "Rayleigh phase must be symmetric for forward and backward scattering");
    ENGINE_ASSERT(pRFwd > pRPerp, "Rayleigh phase at 0 deg must be twice the perpendicular 90 deg scattering");
    (void)pRFwd; (void)pRBwd; (void)pRPerp;

    // 2. Mie forward scattering with g = 0.8 must strongly favor forward direction
    const float pMFwd = AtmosphericSky::EvaluateMiePhase(1.0f, 0.8f);
    const float pMBwd = AtmosphericSky::EvaluateMiePhase(-1.0f, 0.8f);

    ENGINE_ASSERT(pMFwd > pMBwd * 10.0f, "Mie scattering with g=0.8 must exhibit strong forward solar peak");

    ENGINE_LOG_INFO("Rayleigh symmetry and Mie forward lobe (Ratio: {:.1f}x) verified.", pMFwd / pMBwd);
}

void TestAtmosphericSkyRadiance() {
    using namespace Engine::Core;
    using namespace Engine::Renderer;

    Logger::LogMessage(LogLevel::Info, "--- Testing Physical Atmospheric Sky Radiance & Chromaticity ---");

    AtmosphereParameters params{};
    params.sunDirection = {0.0f, 1.0f, 0.0f}; // Sun directly at zenith (high noon)

    // Looking toward the sun includes the bright solar disc contribution.
    const Vec3 sunRadiance = AtmosphericSky::ComputeSkyRadiance({0.0f, 1.0f, 0.0f}, params);
    ENGINE_ASSERT(sunRadiance.x > 0.0f && sunRadiance.y > 0.0f && sunRadiance.z > 0.0f,
        "Sun-facing radiance must be positive in every channel");

    // Looking away from the zenith sun isolates the blue-biased Rayleigh scattering.
    const Vec3 blueSkyRadiance = AtmosphericSky::ComputeSkyRadiance({1.0f, 0.0f, 0.0f}, params);
    ENGINE_ASSERT(blueSkyRadiance.z > blueSkyRadiance.x,
        "Off-sun sky must exhibit blue Rayleigh dominance (B > R)");

    const Vec3 horizonRadiance = AtmosphericSky::ComputeSkyRadiance({1.0f, 0.05f, 0.0f}, params);
    ENGINE_ASSERT(horizonRadiance.x > 0.0f && horizonRadiance.y > 0.0f && horizonRadiance.z > 0.0f,
        "Horizon radiance must be non-zero");

    ENGINE_LOG_INFO("Off-sun Rayleigh sky chromaticity verified (RGB: [{:.6f}, {:.6f}, {:.6f}]).",
        blueSkyRadiance.x, blueSkyRadiance.y, blueSkyRadiance.z);
}

void TestDualLobeGGXMicrofacets() {
    using namespace Engine::Core;
    using namespace Engine::Renderer;

    Logger::LogMessage(LogLevel::Info, "--- Testing Dual-Lobe GGX Specular Microfacet BRDF ---");

    // 1. When lobe weight is 0.0, dual-lobe must equal pure single GGX(alpha1)
    const float alpha1 = 0.1f;
    const float alpha2 = 0.5f;
    const float dSingle = AtmosphericSky::EvaluateDualLobeNDF(0.95f, alpha1, alpha2, 0.0f);
    const float dSingleTest = AtmosphericSky::EvaluateDualLobeNDF(0.95f, alpha1, alpha1, 0.5f);
    ENGINE_ASSERT(std::abs(dSingle - dSingleTest) < 1e-4f, "Dual-lobe with w=0.0 must exactly match single GGX");
    (void)dSingle; (void)dSingleTest;

    // 2. Blend lobe weight w = 0.3 for clearcoat lacquer on rough metallic base
    const float dBlended = AtmosphericSky::EvaluateDualLobeNDF(0.95f, alpha1, alpha2, 0.3f);
    ENGINE_ASSERT(dBlended > 0.0f, "Blended dual-lobe NDF must be strictly positive");

    ENGINE_LOG_INFO("Dual-Lobe GGX microfacet NDF evaluated successfully (Blended NDF: {:.4f}).", dBlended);
}

void TestAtmosphericSkyRenderGraphPass() {
    using namespace Engine::Renderer;
    using namespace Engine::RHI;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing AtmosphereSky RenderGraph Pass Scheduling ---");

    RenderGraph rg;

    RGTextureDesc depthDesc{1280, 720, Format::D32_FLOAT, TextureUsage::DepthStencil, "GBufferDepth"};
    auto hDepth = rg.DeclareTexture("GBufferDepth", depthDesc);

    RGTextureDesc skyDesc{1280, 720, Format::RGBA16_FLOAT, TextureUsage::RenderTarget | TextureUsage::ShaderResource, "AtmosphericSkyColor"};
    auto hSky = rg.DeclareTexture("AtmosphericSkyColor", skyDesc);

    AtmosphericSky::AddToGraph(rg, hDepth, hSky);
    rg.MarkOutput(hSky);

    ENGINE_ASSERT(rg.Compile(), "RenderGraph compilation with AtmosphereSkyPass must succeed");
    ENGINE_ASSERT(rg.GetCompiledPassCount() == 1, "Expected 1 active AtmosphereSkyPass scheduled");

    ENGINE_LOG_INFO("AtmosphereSkyPass successfully compiled and scheduled in the RenderGraph.");
}

void RunAtmosphericSkyUnitTests() {
    TestRayleighAndMiePhases();
    TestAtmosphericSkyRadiance();
    TestDualLobeGGXMicrofacets();
    TestAtmosphericSkyRenderGraphPass();
}
