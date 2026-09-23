#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/MathTypes.h"
#include <vector>

namespace Engine::Renderer {

// Represents a cluster of micro-triangles (typically up to 64 vertices and 126 triangles)
struct MeshletCluster {
    Core::u32 clusterID{0};
    Core::u32 lodLevel{0};
    Core::AABB boundingBox;
    Core::Vec3 boundingSphereCenter{0.0f, 0.0f, 0.0f};
    float boundingSphereRadius{1.0f};

    // Geometric error metric in world space units
    float clusterError{0.0f};
    float parentError{1e9f};

    Core::u32 vertexOffset{0};
    Core::u32 triangleOffset{0};
    Core::u32 triangleCount{0};
};

class MeshletLODDAG {
public:
    MeshletLODDAG() = default;

    // Evaluates whether a cluster is selected for rendering based on projected screen space error
    [[nodiscard]] static bool ShouldRenderCluster(
        const MeshletCluster& cluster,
        const Core::Vec3& cameraPos,
        float screenErrorThresholdPixels,
        float fovY,
        float screenHeight
    ) noexcept;

    // Calculates projected screen space error in pixels
    [[nodiscard]] static float ProjectErrorToScreen(
        float geometricErrorWorld,
        float distanceToCamera,
        float fovY,
        float screenHeight
    ) noexcept;
};

} // namespace Engine::Renderer
