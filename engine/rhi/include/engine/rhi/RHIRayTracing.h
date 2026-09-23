#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/MathTypes.h"
#include "engine/rhi/RHIBuffer.h"
#include <vector>
#include <string>

namespace Engine::RHI {

// DXR 1.1 / Vulkan Acceleration Structure Geometry Flag
enum class GeometryFlags : Core::u32 {
    None = 0,
    Opaque = 1 << 0,
    NoDuplicateAnyHitInvocation = 1 << 1
};

inline GeometryFlags operator|(GeometryFlags a, GeometryFlags b) noexcept {
    return static_cast<GeometryFlags>(static_cast<Core::u32>(a) | static_cast<Core::u32>(b));
}

// Geometry description for Bottom-Level Acceleration Structure (BLAS)
struct RayTracingGeometryDesc {
    RHIBuffer* vertexBuffer{nullptr};
    Core::u32 vertexCount{0};
    Core::u32 vertexStride{sizeof(Core::Vec3)};
    RHIBuffer* indexBuffer{nullptr};
    Core::u32 indexCount{0};
    GeometryFlags flags{GeometryFlags::Opaque};
};

// Instance description for Top-Level Acceleration Structure (TLAS)
struct alignas(16) RayTracingInstanceDesc {
    float transform[3][4]{}; // 3x4 row-major affine transformation matrix (48 bytes)
    Core::u32 instanceID{0}; // 4 bytes
    Core::u8 instanceMask{0xFF}; // 1 byte
    Core::u8 flags{0}; // 1 byte
    Core::u16 pad0{0}; // 2 bytes
    Core::u32 instanceContributionToHitGroupIndex{0}; // 4 bytes
    Core::u32 pad1{0}; // 4 bytes
    Core::u64 accelerationStructureGPUVirtualAddress{0}; // 8 bytes
    Core::u64 pad2{0}; // 8 bytes (total 80 bytes, 16-byte aligned)

    void SetTransform(const Core::Mat4x4& mat) noexcept {
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 4; ++c) {
                transform[r][c] = mat.m[r + c * 4];
            }
        }
    }
};

// Memory requirements for building an acceleration structure
struct AccelerationStructureSizes {
    Core::u64 resultDataSizeInBytes{0};
    Core::u64 scratchDataSizeInBytes{0};
    Core::u64 updateScratchDataSizeInBytes{0};
};

// Shader Binding Table (SBT) entry alignment requirement (64 bytes for DXR)
constexpr Core::u64 DXR_SBT_ENTRY_ALIGNMENT = 64;

struct alignas(64) ShaderRecord {
    Core::u8 shaderIdentifier[32]{};
    Core::u8 localRootArguments[32]{};
};

class RHIAccelerationStructure {
public:
    virtual ~RHIAccelerationStructure() = default;

    [[nodiscard]] virtual bool IsBuilt() const noexcept = 0;
    [[nodiscard]] virtual Core::u64 GetGPUVirtualAddress() const noexcept = 0;
    [[nodiscard]] virtual Core::u64 GetSizeInBytes() const noexcept = 0;
};

} // namespace Engine::RHI
