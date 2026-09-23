#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/MathTypes.h"
#include "engine/renderer/RenderGraph.h"

namespace Engine::Renderer {

// Physical atmospheric density parameters (Earth atmosphere baseline)
struct AtmosphereParameters {
    float planetRadius{6360.0e3f};     // 6360 km
    float atmosphereRadius{6460.0e3f}; // 6460 km (100 km thickness)
    float rayleighScaleHeight{8000.0f}; // 8 km
    float mieScaleHeight{1200.0f};      // 1.2 km

    // Rayleigh scattering coefficients at wavelengths 680nm (R), 550nm (G), 440nm (B)
    Core::Vec3 rayleighScattering{5.802e-6f, 13.558e-6f, 33.100e-6f};

    // Mie scattering coefficients and forward asymmetry factor
    Core::Vec3 mieScattering{3.996e-6f, 3.996e-6f, 3.996e-6f};
    float mieAsymmetryG{0.8f};

    // Solar intensity and direction
    Core::Vec3 sunDirection{0.0f, 0.7071f, 0.7071f};
    Core::Vec3 sunIntensity{20.0f, 20.0f, 20.0f};
};

class AtmosphericSky {
public:
    AtmosphericSky() = default;

    // Evaluates Rayleigh phase function: 3 / (16 * pi) * (1 + cos^2(theta))
    [[nodiscard]] static float EvaluateRayleighPhase(float cosTheta) noexcept;

    // Evaluates Henyey-Greenstein Mie phase function for forward solar glare
    [[nodiscard]] static float EvaluateMiePhase(float cosTheta, float g) noexcept;

    // Computes analytical optical depth through spherical atmospheric shells
    [[nodiscard]] static float ComputeOpticalDepth(
        float startAltitude,
        float cosZenith,
        float scaleHeight,
        float atmosphereThickness = 100.0e3f
    ) noexcept;

    // Evaluates in-scattered solar sky radiance along view ray
    [[nodiscard]] static Core::Vec3 ComputeSkyRadiance(
        const Core::Vec3& viewDir,
        const AtmosphereParameters& params
    ) noexcept;

    // Dual-Lobe GGX Specular Microfacet Model
    // Blends primary specular lobe with secondary clearcoat/sheen lobe
    [[nodiscard]] static float EvaluateDualLobeNDF(
        float NdotH,
        float alpha1,
        float alpha2,
        float lobeWeight
    ) noexcept;

    static void AddToGraph(
        RenderGraph& rg,
        RGTextureHandle depthBuffer,
        RGTextureHandle outSkyRadiance,
        const AtmosphereParameters& params = {}
    );
};

} // namespace Engine::Renderer
