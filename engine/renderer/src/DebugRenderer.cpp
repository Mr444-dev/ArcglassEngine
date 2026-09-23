#include "engine/renderer/DebugRenderer.h"
#include "engine/rhi/RHIDevice.h"
#include "engine/core/Log.h"
#include <cmath>
#include <cstring>

namespace Engine::Renderer {

bool DebugRenderer::Initialize() {
    auto& device = RHI::RHIDevice::Get();

    RHI::BufferDesc bDesc{};
    bDesc.sizeInBytes = sizeof(DebugVertex) * MAX_DEBUG_LINES * 2;
    bDesc.usage = RHI::BufferUsage::Vertex | RHI::BufferUsage::Upload;
    bDesc.debugName = "DebugLineVertexBuffer";

    m_vertexBuffer = device.CreateBuffer(bDesc);
    if (!m_vertexBuffer) {
        ENGINE_LOG_ERROR("Failed to create Debug line vertex buffer");
        return false;
    }

    m_vertices.reserve(1024);
    ENGINE_LOG_INFO("DebugRenderer initialized (Capacity: {} lines).", MAX_DEBUG_LINES);
    return true;
}

void DebugRenderer::Shutdown() {
    m_vertexBuffer.reset();
    m_pso.reset();
    m_vertices.clear();
}

void DebugRenderer::AddLine(const Vec3& start, const Vec3& end, const float color[4]) {
    if (m_vertices.size() + 2 > MAX_DEBUG_LINES * 2) return;

    DebugVertex v0{start, {color[0], color[1], color[2], color[3]}};
    DebugVertex v1{end,   {color[0], color[1], color[2], color[3]}};

    m_vertices.push_back(v0);
    m_vertices.push_back(v1);
}

void DebugRenderer::AddAABB(const AABB& aabb, const float color[4]) {
    const Vec3& min = aabb.min;
    const Vec3& max = aabb.max;

    // Bottom 4 lines
    AddLine({min.x, min.y, min.z}, {max.x, min.y, min.z}, color);
    AddLine({max.x, min.y, min.z}, {max.x, min.y, max.z}, color);
    AddLine({max.x, min.y, max.z}, {min.x, min.y, max.z}, color);
    AddLine({min.x, min.y, max.z}, {min.x, min.y, min.z}, color);

    // Top 4 lines
    AddLine({min.x, max.y, min.z}, {max.x, max.y, min.z}, color);
    AddLine({max.x, max.y, min.z}, {max.x, max.y, max.z}, color);
    AddLine({max.x, max.y, max.z}, {min.x, max.y, max.z}, color);
    AddLine({min.x, max.y, max.z}, {min.x, max.y, min.z}, color);

    // Vertical 4 pillars
    AddLine({min.x, min.y, min.z}, {min.x, max.y, min.z}, color);
    AddLine({max.x, min.y, min.z}, {max.x, max.y, min.z}, color);
    AddLine({max.x, min.y, max.z}, {max.x, max.y, max.z}, color);
    AddLine({min.x, min.y, max.z}, {min.x, max.y, max.z}, color);
}

void DebugRenderer::AddSphere(const Vec3& center, float radius, const float color[4], Core::u32 segments) {
    if (segments < 4) segments = 4;
    const float step = (2.0f * 3.14159265f) / static_cast<float>(segments);

    for (Core::u32 i = 0; i < segments; ++i) {
        const float a0 = i * step;
        const float a1 = (i + 1) * step;

        // XY plane
        AddLine(
            {center.x + radius * std::cos(a0), center.y + radius * std::sin(a0), center.z},
            {center.x + radius * std::cos(a1), center.y + radius * std::sin(a1), center.z},
            color
        );
        // XZ plane
        AddLine(
            {center.x + radius * std::cos(a0), center.y, center.z + radius * std::sin(a0)},
            {center.x + radius * std::cos(a1), center.y, center.z + radius * std::sin(a1)},
            color
        );
        // YZ plane
        AddLine(
            {center.x, center.y + radius * std::cos(a0), center.z + radius * std::sin(a0)},
            {center.x, center.y + radius * std::cos(a1), center.z + radius * std::sin(a1)},
            color
        );
    }
}

void DebugRenderer::Flush(RHI::RHICommandList& cmdList, const Camera& camera) {
    (void)cmdList;
    (void)camera;

    if (m_vertices.empty() || !m_vertexBuffer) return;

    void* mapped = m_vertexBuffer->Map();
    if (mapped) {
        const size_t byteSize = sizeof(DebugVertex) * m_vertices.size();
        std::memcpy(mapped, m_vertices.data(), byteSize);
        m_vertexBuffer->Unmap();
    }
}

void DebugRenderer::Clear() noexcept {
    m_vertices.clear();
}

} // namespace Engine::Renderer
