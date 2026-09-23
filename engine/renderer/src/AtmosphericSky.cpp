#include "engine/renderer/AtmosphericSky.h"
#include <cmath>
#include <algorithm>

namespace Engine::Renderer {

constexpr float PI = 3.14159265358979323846f;
constexpr float INV_4PI = 1.0f / (4.0f * PI);

float AtmosphericSky::EvaluateRayleighPhase(float cosTheta) noexcept {
    constexpr float k = 3.0f / (16.0f * PI);
    return k * (1.0f + cosTheta * cosTheta);
}

float AtmosphericSky::EvaluateMiePhase(float cosTheta, float g) noexcept {
    const float g2 = g * g;
    const float denom = 1.0f + g2 - 2.0f * g * cosTheta;
    if (denom <= 0.0f) return 0.0f;
    return INV_4PI * ((1.0f - g2) / std::pow(denom, 1.5f));
}

float AtmosphericSky::ComputeOpticalDepth(
    float startAltitude,
    float cosZenith,
    float scaleHeight,
    float atmosphereThickness) noexcept {
    if (scaleHeight <= 0.0f) return 0.0f;

    // Numerical integration along zenith angle through exponential density profile
    const float clampedCos = std::max(cosZenith, 0.05f);
    const float baseDensity = std::exp(-startAltitude / scaleHeight);
    const float pathLength = atmosphereThickness / clampedCos;

    return baseDensity * scaleHeight * (1.0f - std::exp(-pathLength / scaleHeight));
}

Core::Vec3 AtmosphericSky::ComputeSkyRadiance(
    const Core::Vec3& viewDir,
    const AtmosphereParameters& params) noexcept {
    const Core::Vec3 sunDir = params.sunDirection.Normalized();
    const Core::Vec3 vDir = viewDir.Normalized();

    const float cosTheta = vDir.Dot(sunDir);
    const float cosZenith = std::max(vDir.y, 0.02f);

    const float optRayleigh = ComputeOpticalDepth(0.0f, cosZenith, params.rayleighScaleHeight);
    const float optMie = ComputeOpticalDepth(0.0f, cosZenith, params.mieScaleHeight);

    // Beer-Lambert transmittance extinction: tau = exp(- (beta_R * opt_R + beta_M * opt_M))
    const Core::Vec3 tau = {
        std::exp(-(params.rayleighScattering.x * optRayleigh + params.mieScattering.x * optMie)),
        std::exp(-(params.rayleighScattering.y * optRayleigh + params.mieScattering.y * optMie)),
        std::exp(-(params.rayleighScattering.z * optRayleigh + params.mieScattering.z * optMie))
    };

    const float pRayleigh = EvaluateRayleighPhase(cosTheta);
    const float pMie = EvaluateMiePhase(cosTheta, params.mieAsymmetryG);

    // In-scattered radiance
    const Core::Vec3 inScatter = {
        (params.rayleighScattering.x * pRayleigh + params.mieScattering.x * pMie) * params.sunIntensity.x * (1.0f - tau.x),
        (params.rayleighScattering.y * pRayleigh + params.mieScattering.y * pMie) * params.sunIntensity.y * (1.0f - tau.y),
        (params.rayleighScattering.z * pRayleigh + params.mieScattering.z * pMie) * params.sunIntensity.z * (1.0f - tau.z)
    };

    // Sun disc direct contribution
    Core::Vec3 sunDisc{0.0f, 0.0f, 0.0f};
    if (cosTheta > 0.9995f) {
        sunDisc = {params.sunIntensity.x * tau.x * 20.0f,
                   params.sunIntensity.y * tau.y * 20.0f,
                   params.sunIntensity.z * tau.z * 20.0f};
    }

    return inScatter + sunDisc;
}

float AtmosphericSky::EvaluateDualLobeNDF(
    float NdotH,
    float alpha1,
    float alpha2,
    float lobeWeight) noexcept {
    const float clampedNdotH = std::clamp(NdotH, 0.0f, 1.0f);
    const float NdotH2 = clampedNdotH * clampedNdotH;

    auto evalGGX = [NdotH2](float alpha) -> float {
        const float a2 = std::max(alpha * alpha, 1e-4f);
        const float d = NdotH2 * (a2 - 1.0f) + 1.0f;
        return a2 / (PI * d * d);
    };

    const float d1 = evalGGX(alpha1);
    const float d2 = evalGGX(alpha2);
    const float w = std::clamp(lobeWeight, 0.0f, 1.0f);

    return (1.0f - w) * d1 + w * d2;
}

void AtmosphericSky::AddToGraph(
    RenderGraph& rg,
    RGTextureHandle depthBuffer,
    RGTextureHandle outSkyRadiance,
    const AtmosphereParameters& params) {
    (void)params;
    rg.AddPass("AtmosphereSkyPass",
        [&](RGPassBuilder& builder) {
            builder.ReadTexture(depthBuffer, RHI::ResourceState::ShaderResource);
            builder.WriteTexture(outSkyRadiance, RHI::ResourceState::RenderTarget);
        },
        [outSkyRadiance](RHI::RHICommandList& cmdList, const RenderGraph& graph) {
            const auto* outTex = graph.GetPhysicalTexture(outSkyRadiance);
            if (outTex) {
                const Core::u32 threadsX = (outTex->GetWidth() + 7) / 8;
                const Core::u32 threadsY = (outTex->GetHeight() + 7) / 8;
                cmdList.DispatchCompute(threadsX, threadsY, 1);
            }
        }
    );
}

} // namespace Engine::Renderer
