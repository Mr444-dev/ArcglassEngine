#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/renderer/Camera.h"
#include "engine/rhi/RHICommandList.h"
#include "engine/rhi/RHIBuffer.h"
#include <vector>
#include <memory>

namespace Engine::Renderer {

struct DebugVertex {
    Vec3 position;
    float color[4];
};

class DebugRenderer {
public:
    static constexpr Core::u32 MAX_DEBUG_LINES = 65536;

    DebugRenderer() = default;
    ~DebugRenderer() = default;

    bool Initialize();
    void Shutdown();

    void AddLine(const Vec3& start, const Vec3& end, const float color[4]);
    void AddAABB(const AABB& aabb, const float color[4]);
    void AddSphere(const Vec3& center, float radius, const float color[4], Core::u32 segments = 16);

    void Flush(RHI::RHICommandList& cmdList, const Camera& camera);
    void Clear() noexcept;

    [[nodiscard]] size_t GetLineCount() const noexcept { return m_vertices.size() / 2; }

private:
    std::vector<DebugVertex> m_vertices;
    std::unique_ptr<RHI::RHIBuffer> m_vertexBuffer;
    std::unique_ptr<RHI::RHIPipelineState> m_pso;
};

} // namespace Engine::Renderer
