#include "engine/renderer/MeshletLODDAG.h"
#include <cmath>
#include <algorithm>

namespace Engine::Renderer {

float MeshletLODDAG::ProjectErrorToScreen(
    float geometricErrorWorld,
    float distanceToCamera,
    float fovY,
    float screenHeight
) noexcept {
    if (distanceToCamera < 1e-3f) return 1e6f;
    const float halfFovTan = std::tan(0.5f * fovY);
    if (halfFovTan < 1e-4f) return 1e6f;

    // Projected pixel size = (worldSize * screenHeight) / (2.0 * distance * tan(fov / 2))
    return (geometricErrorWorld * screenHeight) / (2.0f * distanceToCamera * halfFovTan);
}

bool MeshletLODDAG::ShouldRenderCluster(
    const MeshletCluster& cluster,
    const Core::Vec3& cameraPos,
    float screenErrorThresholdPixels,
    float fovY,
    float screenHeight
) noexcept {
    const Core::Vec3 toCenter = cluster.boundingSphereCenter - cameraPos;
    const float rawDist = toCenter.Length();
    const float conservativeDist = std::max(rawDist - cluster.boundingSphereRadius, 0.01f);

    const float currentError = ProjectErrorToScreen(cluster.clusterError, conservativeDist, fovY, screenHeight);
    const float parentError = ProjectErrorToScreen(cluster.parentError, conservativeDist, fovY, screenHeight);

    // Continuous DAG Cut Rule:
    // A cluster is rendered if and only if its own error is below the screen threshold,
    // but its parent's error was greater than the threshold.
    return (currentError <= screenErrorThresholdPixels) && (parentError > screenErrorThresholdPixels);
}

} // namespace Engine::Renderer
