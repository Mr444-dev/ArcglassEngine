#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/MathTypes.h"
#include <array>
#include <vector>

namespace Engine::Assets {

// 128-bit Globally Unique Identifier
struct AssetGUID {
    Core::u64 high{0};
    Core::u64 low{0};

    [[nodiscard]] bool IsValid() const noexcept { return high != 0 || low != 0; }
    [[nodiscard]] bool operator==(const AssetGUID& o) const noexcept { return high == o.high && low == o.low; }
    [[nodiscard]] bool operator!=(const AssetGUID& o) const noexcept { return !(*this == o); }

    [[nodiscard]] std::string ToString() const;
    static AssetGUID FromString(const std::string& str);
    static AssetGUID CreateRandom();
    static AssetGUID FromContent(const void* data, size_t size);
};

// Common Binary Header across all .c* assets
struct alignas(16) AssetFileHeader {
    Core::u32 magic{0};         // 'CMSH', 'CTEX', 'CMAT'
    Core::u32 version{1};       // Format version
    Core::u64 padding{0};       // 16-byte alignment pad
    AssetGUID guid{};           // Unique asset ID (16 bytes)
    Core::u64 uncompressedSize{0};
    Core::u64 compressedSize{0}; // 0 if uncompressed
};

// ==========================================
// Custom Mesh Binary Format (.cmesh)
// Magic: 'CMSH' (0x48534D43)
// ==========================================
inline constexpr Core::u32 CMESH_MAGIC = 0x48534D43;

struct MeshVertex {
    Core::Vec3 position{0.0f, 0.0f, 0.0f};
    Core::Vec3 normal{0.0f, 1.0f, 0.0f};
    float tangent[4]{1.0f, 0.0f, 0.0f, 1.0f};
    float uv[2]{0.0f, 0.0f};
};

struct MeshletDescriptor {
    Core::u32 vertexOffset{0};
    Core::u32 triangleOffset{0};
    Core::u32 vertexCount{0};
    Core::u32 triangleCount{0};
    float boundingSphere[4]{0.0f, 0.0f, 0.0f, 0.0f}; // Center (xyz), Radius (w)
    float coneNormal[3]{0.0f, 0.0f, 0.0f};
    float coneCutoff{0.0f};
};

struct SubmeshDescriptor {
    Core::u32 vertexOffset{0};
    Core::u32 vertexCount{0};
    Core::u32 indexOffset{0};
    Core::u32 indexCount{0};
    Core::u32 meshletOffset{0};
    Core::u32 meshletCount{0};
    Core::u32 materialIndex{0};
    Core::AABB aabb;
};

struct MeshFileHeader {
    AssetFileHeader commonHeader;
    Core::u32 submeshCount{0};
    Core::u32 totalVertexCount{0};
    Core::u32 totalIndexCount{0};
    Core::u32 totalMeshletCount{0};
    Core::AABB totalBounds;
};

// ==========================================
// Custom Texture Binary Format (.ctex)
// Magic: 'CTEX' (0x58455443)
// ==========================================
inline constexpr Core::u32 CTEX_MAGIC = 0x58455443;

enum class TextureCompressionFormat : Core::u8 {
    RGBA8_UNORM = 0,
    BC1_RGB = 1,      // DXT1
    BC3_RGBA = 2,     // DXT5
    BC4_R = 3,        // Single-channel (e.g., Metallic/Roughness/Height)
    BC5_RG = 4,       // Normal maps (Tangent space XY)
    BC7_RGBA = 5      // High-quality HDR/PBR albedo
};

struct MipLevelDescriptor {
    Core::u64 fileOffset{0};
    Core::u64 byteSize{0};
    Core::u32 width{0};
    Core::u32 height{0};
    Core::u32 rowPitch{0};
};

struct TextureFileHeader {
    AssetFileHeader commonHeader;
    Core::u32 width{1};
    Core::u32 height{1};
    Core::u32 depth{1};
    Core::u32 mipCount{1};
    TextureCompressionFormat format{TextureCompressionFormat::RGBA8_UNORM};
    std::array<MipLevelDescriptor, 16> mips{};
};

// ==========================================
// Custom Material Binary Format (.cmat)
// Magic: 'CMAT' (0x54414D43)
// ==========================================
inline constexpr Core::u32 CMAT_MAGIC = 0x54414D43;

struct MaterialFileHeader {
    AssetFileHeader commonHeader;
    AssetGUID albedoTextureGUID{};
    AssetGUID normalTextureGUID{};
    AssetGUID metallicRoughnessTextureGUID{};
    AssetGUID emissiveTextureGUID{};

    float baseColorFactor[4]{1.0f, 1.0f, 1.0f, 1.0f};
    float roughnessFactor{1.0f};
    float metallicFactor{0.0f};
    float emissiveFactor[3]{0.0f, 0.0f, 0.0f};
    float alphaCutoff{0.5f};
};

} // namespace Engine::Assets
