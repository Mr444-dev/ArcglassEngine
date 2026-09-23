#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/rhi/RHITypes.h"

namespace Engine::RHI::Vulkan {

// Vulkan 1.3 Core & Extension Feature Descriptors
struct VulkanCapabilities {
    bool hasDescriptorIndexing{true}; // VK_EXT_descriptor_indexing
    bool hasDynamicRendering{true};   // VK_KHR_dynamic_rendering
    bool hasSynchronization2{true};   // VK_KHR_synchronization2
    bool hasRayTracing{true};          // VK_KHR_ray_tracing_pipeline & acceleration_structure
    bool hasMeshShaders{true};         // VK_EXT_mesh_shader
    Core::u32 maxBindlessSamplers{500000};
    Core::u32 maxBindlessSampledImages{500000};
};

// Maps generic RHI Format to Vulkan VkFormat representation
[[nodiscard]] constexpr Core::u32 ToVkFormat(Format format) noexcept {
    switch (format) {
        case Format::RGBA8_UNORM:      return 37;  // VK_FORMAT_R8G8B8A8_UNORM
        case Format::RGBA8_UNORM_SRGB: return 43;  // VK_FORMAT_R8G8B8A8_SRGB
        case Format::RGBA16_FLOAT:     return 97;  // VK_FORMAT_R16G16B16A16_SFLOAT
        case Format::RGB32_FLOAT:      return 106; // VK_FORMAT_R32G32B32_SFLOAT
        case Format::D32_FLOAT:        return 126; // VK_FORMAT_D32_SFLOAT
        case Format::R32_FLOAT:        return 100; // VK_FORMAT_R32_SFLOAT
        case Format::Unknown:
        default:                       return 0;   // VK_FORMAT_UNDEFINED
    }
}

// Maps generic RHI ResourceState to Vulkan 1.3 Synchronization2 Pipeline Stage & Access Flags
struct VkBarrierMapping {
    Core::u64 stageFlags{0};
    Core::u64 accessFlags{0};
    Core::u32 imageLayout{0};
};

[[nodiscard]] inline VkBarrierMapping ToVkBarrier(ResourceState state) noexcept {
    VkBarrierMapping mapping{};
    if (state & ResourceState::RenderTarget) {
        mapping.stageFlags = 0x00000080; // VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
        mapping.accessFlags = 0x00000100; // VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
        mapping.imageLayout = 2; // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    } else if (state & ResourceState::DepthWrite) {
        mapping.stageFlags = 0x00000200; // VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT
        mapping.accessFlags = 0x00000400; // VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
        mapping.imageLayout = 3; // VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    } else if (state & ResourceState::ShaderResource) {
        mapping.stageFlags = 0x00000040; // VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT
        mapping.accessFlags = 0x00000020; // VK_ACCESS_2_SHADER_READ_BIT
        mapping.imageLayout = 5; // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    } else if (state & ResourceState::UnorderedAccess) {
        mapping.stageFlags = 0x00000020; // VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
        mapping.accessFlags = 0x00000040 | 0x00000080; // READ | WRITE
        mapping.imageLayout = 6; // VK_IMAGE_LAYOUT_GENERAL
    } else {
        mapping.stageFlags = 0x00000001; // VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT
        mapping.accessFlags = 0;
        mapping.imageLayout = 0; // VK_IMAGE_LAYOUT_UNDEFINED
    }
    return mapping;
}

} // namespace Engine::RHI::Vulkan
