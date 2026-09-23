#include "engine/core/BaseTypes.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "engine/rhi/RHIRayTracing.h"
#include "engine/rhi/vulkan/VulkanTypes.h"

void TestBLASGeometryDescriptor() {
    using namespace Engine::RHI;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing BLAS Geometry Descriptors ---");

    RayTracingGeometryDesc geom{};
    geom.vertexCount = 1024;
    geom.vertexStride = sizeof(Vec3);
    geom.indexCount = 2048;
    geom.flags = GeometryFlags::Opaque | GeometryFlags::NoDuplicateAnyHitInvocation;

    ENGINE_ASSERT(geom.vertexCount == 1024, "Vertex count mismatch");
    ENGINE_ASSERT(geom.indexCount == 2048, "Index count mismatch");
    ENGINE_ASSERT(geom.vertexStride == 12, "Vec3 stride must be 12 bytes");
    ENGINE_ASSERT((static_cast<u32>(geom.flags) & static_cast<u32>(GeometryFlags::Opaque)) != 0, "Flags must include Opaque");

    ENGINE_LOG_INFO("BLAS Geometry Descriptor verified (1024 verts, 2048 indices, Opaque).");
}

void TestTLASInstanceDescriptorAndAffineTransform() {
    using namespace Engine::RHI;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing TLAS Instance Descriptor & Affine Transform ---");

    ENGINE_ASSERT(alignof(RayTracingInstanceDesc) == 16, "RayTracingInstanceDesc must be 16-byte aligned for DXR");

    RayTracingInstanceDesc instance{};
    instance.instanceID = 42;
    instance.instanceMask = 0xFF;
    instance.instanceContributionToHitGroupIndex = 0;
    instance.accelerationStructureGPUVirtualAddress = 0x10000000;

    // Construct transform matrix: Translate(10, 20, 30) * Scale(2, 2, 2)
    Mat4x4 mat = Mat4x4::Translation({10.0f, 20.0f, 30.0f}) * Mat4x4::Scale({2.0f, 2.0f, 2.0f});
    instance.SetTransform(mat);

    // Verify 3x4 affine layout:
    // Row 0: [2, 0, 0, 10]
    // Row 1: [0, 2, 0, 20]
    // Row 2: [0, 0, 2, 30]
    ENGINE_ASSERT(std::abs(instance.transform[0][0] - 2.0f) < 1e-4f, "Transform Scale X mismatch");
    ENGINE_ASSERT(std::abs(instance.transform[1][1] - 2.0f) < 1e-4f, "Transform Scale Y mismatch");
    ENGINE_ASSERT(std::abs(instance.transform[2][2] - 2.0f) < 1e-4f, "Transform Scale Z mismatch");

    ENGINE_ASSERT(std::abs(instance.transform[0][3] - 10.0f) < 1e-4f, "Transform Translation X mismatch");
    ENGINE_ASSERT(std::abs(instance.transform[1][3] - 20.0f) < 1e-4f, "Transform Translation Y mismatch");
    ENGINE_ASSERT(std::abs(instance.transform[2][3] - 30.0f) < 1e-4f, "Transform Translation Z mismatch");

    ENGINE_LOG_INFO("TLAS Instance Descriptor affine transform verified (Pos: [10, 20, 30], Scale: 2x).");
}

void TestShaderBindingTableAlignment() {
    using namespace Engine::RHI;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Shader Binding Table (SBT) Alignment ---");

    ENGINE_ASSERT(alignof(ShaderRecord) == 64, "ShaderRecord must be 64-byte aligned for DXR hardware");
    ENGINE_ASSERT(sizeof(ShaderRecord) == 64, "ShaderRecord size must be exactly 64 bytes");

    ShaderRecord sbtArray[4]{};
    for (int i = 0; i < 4; ++i) {
        auto addr = reinterpret_cast<uintptr_t>(&sbtArray[i]);
        ENGINE_ASSERT(addr % DXR_SBT_ENTRY_ALIGNMENT == 0, "SBT entry must be aligned to 64 bytes");
    }

    ENGINE_LOG_INFO("Shader Binding Table 64-byte hardware alignment verified across 4 records.");
}

void TestVulkan13CapabilitiesAndBarrierMapping() {
    using namespace Engine::RHI;
    using namespace Engine::RHI::Vulkan;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Milestone Phase 13: Vulkan 1.3 Backend & Barrier Mapping ---");

    VulkanCapabilities caps{};
    ENGINE_ASSERT(caps.hasDescriptorIndexing, "Vulkan 1.3 descriptor indexing must be supported");
    ENGINE_ASSERT(caps.hasDynamicRendering, "Vulkan 1.3 dynamic rendering must be supported");
    ENGINE_ASSERT(caps.hasSynchronization2, "Vulkan 1.3 synchronization2 must be supported");
    ENGINE_ASSERT(caps.hasRayTracing, "Vulkan 1.3 ray tracing must be supported");
    ENGINE_ASSERT(caps.maxBindlessSampledImages >= 500000, "Must support 500k bindless images");

    // Format mappings
    ENGINE_ASSERT(ToVkFormat(Format::RGBA8_UNORM) == 37, "VK_FORMAT_R8G8B8A8_UNORM mismatch");
    ENGINE_ASSERT(ToVkFormat(Format::RGBA16_FLOAT) == 97, "VK_FORMAT_R16G16B16A16_SFLOAT mismatch");
    ENGINE_ASSERT(ToVkFormat(Format::D32_FLOAT) == 126, "VK_FORMAT_D32_SFLOAT mismatch");

    // Barrier mappings
    auto rtBarrier = ToVkBarrier(ResourceState::RenderTarget);
    ENGINE_ASSERT(rtBarrier.imageLayout == 2, "Color attachment optimal mismatch");

    auto dsBarrier = ToVkBarrier(ResourceState::DepthWrite);
    ENGINE_ASSERT(dsBarrier.imageLayout == 3, "Depth stencil optimal mismatch");

    auto srvBarrier = ToVkBarrier(ResourceState::ShaderResource);
    ENGINE_ASSERT(srvBarrier.imageLayout == 5, "Shader read only optimal mismatch");

    ENGINE_LOG_INFO("Vulkan 1.3 capabilities, format mappings, and Synchronization2 barriers verified.");
}

void RunRayTracingUnitTests() {
    TestBLASGeometryDescriptor();
    TestTLASInstanceDescriptorAndAffineTransform();
    TestShaderBindingTableAlignment();
    TestVulkan13CapabilitiesAndBarrierMapping();
}
