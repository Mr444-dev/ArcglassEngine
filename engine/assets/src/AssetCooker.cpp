#include "engine/assets/AssetCooker.h"
#include "engine/assets/VirtualFileSystem.h"
#include "engine/core/Log.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

namespace Engine::Assets {
namespace {

bool AddSize(size_t& total, size_t amount) noexcept {
    if (amount > std::numeric_limits<size_t>::max() - total) return false;
    total += amount;
    return true;
}

bool MultiplySize(size_t count, size_t elementSize, size_t& result) noexcept {
    if (elementSize != 0 && count > std::numeric_limits<size_t>::max() / elementSize) return false;
    result = count * elementSize;
    return true;
}

bool IsFinite(const Core::Vec3& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

} // namespace

std::vector<MeshletDescriptor> AssetCooker::GenerateMeshlets(
    const std::vector<MeshVertex>& vertices,
    const std::vector<Core::u32>& indices
) {
    std::vector<MeshletDescriptor> meshlets;
    if (indices.empty() || indices.size() % 3 != 0) return meshlets;

    constexpr Core::u32 MaxVerticesPerMeshlet = 64;
    constexpr Core::u32 MaxTrianglesPerMeshlet = 126;
    const size_t triangleCount = indices.size() / 3;
    meshlets.reserve((triangleCount + MaxTrianglesPerMeshlet - 1) / MaxTrianglesPerMeshlet);

    size_t firstTriangle = 0;
    while (firstTriangle < triangleCount) {
        const size_t batchStart = firstTriangle;
        Core::u32 minVertex = std::numeric_limits<Core::u32>::max();
        Core::u32 maxVertex = 0;

        while (firstTriangle < triangleCount && firstTriangle - batchStart < MaxTrianglesPerMeshlet) {
            Core::u32 triangleMin = std::numeric_limits<Core::u32>::max();
            Core::u32 triangleMax = 0;
            for (size_t corner = 0; corner < 3; ++corner) {
                const Core::u32 index = indices[firstTriangle * 3 + corner];
                triangleMin = std::min(triangleMin, index);
                triangleMax = std::max(triangleMax, index);
            }

            const Core::u32 nextMin = std::min(minVertex, triangleMin);
            const Core::u32 nextMax = std::max(maxVertex, triangleMax);
            if (static_cast<Core::u64>(nextMax) - nextMin + 1 > MaxVerticesPerMeshlet) {
                break;
            }

            minVertex = nextMin;
            maxVertex = nextMax;
            ++firstTriangle;
        }

        if (firstTriangle == batchStart) {
            ENGINE_LOG_ERROR("AssetCooker: A triangle spans more than {} contiguous vertices; meshlet format cannot represent it.", MaxVerticesPerMeshlet);
            return {};
        }

        MeshletDescriptor meshlet{};
        meshlet.vertexOffset = minVertex;
        meshlet.vertexCount = maxVertex - minVertex + 1;
        meshlet.triangleOffset = static_cast<Core::u32>(batchStart * 3);
        meshlet.triangleCount = static_cast<Core::u32>(firstTriangle - batchStart);

        Core::Vec3 center{0.0f, 0.0f, 0.0f};
        const size_t firstIndex = batchStart * 3;
        const size_t indexCount = (firstTriangle - batchStart) * 3;
        for (size_t i = 0; i < indexCount; ++i) {
            center = center + vertices[indices[firstIndex + i]].position;
        }
        center = center * (1.0f / static_cast<float>(indexCount));

        float maxRadiusSquared = 0.0f;
        for (size_t i = 0; i < indexCount; ++i) {
            const Core::Vec3 delta = vertices[indices[firstIndex + i]].position - center;
            const float radiusSquared = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
            maxRadiusSquared = std::max(maxRadiusSquared, radiusSquared);
        }

        meshlet.boundingSphere[0] = center.x;
        meshlet.boundingSphere[1] = center.y;
        meshlet.boundingSphere[2] = center.z;
        meshlet.boundingSphere[3] = std::sqrt(maxRadiusSquared);
        meshlets.push_back(meshlet);
    }

    return meshlets;
}

bool AssetCooker::CookMesh(
    const std::vector<MeshVertex>& vertices,
    const std::vector<Core::u32>& indices,
    const std::string& outVirtualPath,
    AssetGUID guid
) {
    constexpr size_t MaxSerializedCount = std::numeric_limits<Core::u32>::max();
    if (vertices.empty() || indices.empty() || indices.size() % 3 != 0 ||
        vertices.size() > MaxSerializedCount || indices.size() > MaxSerializedCount) {
        ENGINE_LOG_ERROR("AssetCooker: Mesh must contain representable vertices and a non-empty triangle index list.");
        return false;
    }

    for (const auto& vertex : vertices) {
        if (!IsFinite(vertex.position) || !IsFinite(vertex.normal) ||
            !std::isfinite(vertex.tangent[0]) || !std::isfinite(vertex.tangent[1]) ||
            !std::isfinite(vertex.tangent[2]) || !std::isfinite(vertex.tangent[3]) ||
            !std::isfinite(vertex.uv[0]) || !std::isfinite(vertex.uv[1])) {
            ENGINE_LOG_ERROR("AssetCooker: Mesh contains a non-finite vertex attribute.");
            return false;
        }
    }
    for (const Core::u32 index : indices) {
        if (index >= vertices.size()) {
            ENGINE_LOG_ERROR("AssetCooker: Mesh index {} is outside the {}-vertex array.", index, vertices.size());
            return false;
        }
    }

    Core::AABB totalBounds{vertices.front().position, vertices.front().position};
    for (const auto& vertex : vertices) {
        totalBounds.Encapsulate(Core::AABB{vertex.position, vertex.position});
    }

    std::vector<MeshletDescriptor> meshlets = GenerateMeshlets(vertices, indices);
    if (meshlets.empty() || meshlets.size() > MaxSerializedCount) {
        ENGINE_LOG_ERROR("AssetCooker: Failed to generate representable meshlets.");
        return false;
    }

    SubmeshDescriptor submesh{};
    submesh.vertexCount = static_cast<Core::u32>(vertices.size());
    submesh.indexCount = static_cast<Core::u32>(indices.size());
    submesh.meshletCount = static_cast<Core::u32>(meshlets.size());
    submesh.aabb = totalBounds;

    MeshFileHeader header{};
    header.commonHeader.magic = CMESH_MAGIC;
    header.commonHeader.version = 1;
    header.commonHeader.guid = guid;
    header.submeshCount = 1;
    header.totalVertexCount = static_cast<Core::u32>(vertices.size());
    header.totalIndexCount = static_cast<Core::u32>(indices.size());
    header.totalMeshletCount = static_cast<Core::u32>(meshlets.size());
    header.totalBounds = totalBounds;

    size_t meshletSize = 0;
    size_t vertexSize = 0;
    size_t indexSize = 0;
    if (!MultiplySize(meshlets.size(), sizeof(MeshletDescriptor), meshletSize) ||
        !MultiplySize(vertices.size(), sizeof(MeshVertex), vertexSize) ||
        !MultiplySize(indices.size(), sizeof(Core::u32), indexSize)) {
        ENGINE_LOG_ERROR("AssetCooker: Mesh serialization size overflow.");
        return false;
    }

    size_t totalBytes = sizeof(MeshFileHeader);
    if (!AddSize(totalBytes, sizeof(SubmeshDescriptor)) || !AddSize(totalBytes, meshletSize) ||
        !AddSize(totalBytes, vertexSize) || !AddSize(totalBytes, indexSize)) {
        ENGINE_LOG_ERROR("AssetCooker: Mesh serialization size overflow.");
        return false;
    }
    header.commonHeader.uncompressedSize = totalBytes;

    std::vector<Core::u8> buffer(totalBytes);
    Core::u8* cursor = buffer.data();
    std::memcpy(cursor, &header, sizeof(header)); cursor += sizeof(header);
    std::memcpy(cursor, &submesh, sizeof(submesh)); cursor += sizeof(submesh);
    std::memcpy(cursor, meshlets.data(), meshletSize); cursor += meshletSize;
    std::memcpy(cursor, vertices.data(), vertexSize); cursor += vertexSize;
    std::memcpy(cursor, indices.data(), indexSize);

    if (!VirtualFileSystem::Get().WriteFile(outVirtualPath, buffer.data(), buffer.size())) {
        ENGINE_LOG_ERROR("AssetCooker: Failed writing cooked mesh to '{}'.", outVirtualPath);
        return false;
    }

    AssetMetadata metadata{};
    metadata.guid = guid;
    metadata.cookedPath = outVirtualPath;
    metadata.assetType = "Mesh";
    AssetDatabase::Get().RegisterAsset(metadata);

    ENGINE_LOG_INFO("AssetCooker: Cooked mesh '{}' ({} verts, {} tris, {} meshlets, {:.2f} KB)",
        outVirtualPath, vertices.size(), indices.size() / 3, meshlets.size(), totalBytes / 1024.0f);
    return true;
}

bool AssetCooker::CookTexture(
    Core::u32 width,
    Core::u32 height,
    const Core::u8* rgbaPixels,
    size_t rgbaByteCount,
    const std::string& outVirtualPath,
    AssetGUID guid
) {
    constexpr Core::u32 MaxTextureDimension = 16384;
    if (width == 0 || height == 0 || width > MaxTextureDimension || height > MaxTextureDimension || !rgbaPixels) {
        ENGINE_LOG_ERROR("AssetCooker: Invalid or unsupported texture dimensions {}x{}.", width, height);
        return false;
    }

    const Core::u64 expectedByteCount64 = static_cast<Core::u64>(width) * height * 4;
    if (expectedByteCount64 > std::numeric_limits<size_t>::max() ||
        rgbaByteCount != static_cast<size_t>(expectedByteCount64)) {
        ENGINE_LOG_ERROR("AssetCooker: RGBA input byte count does not match {}x{} texture dimensions.", width, height);
        return false;
    }

    TextureFileHeader header{};
    header.commonHeader.magic = CTEX_MAGIC;
    header.commonHeader.version = 1;
    header.commonHeader.guid = guid;
    header.width = width;
    header.height = height;
    header.depth = 1;
    header.format = TextureCompressionFormat::RGBA8_UNORM;

    std::vector<std::vector<Core::u8>> mipData;
    Core::u32 currentW = width;
    Core::u32 currentH = height;
    mipData.emplace_back(rgbaPixels, rgbaPixels + rgbaByteCount);

    while (currentW > 1 || currentH > 1) {
        const Core::u32 nextW = std::max(1u, currentW / 2);
        const Core::u32 nextH = std::max(1u, currentH / 2);
        const auto& previous = mipData.back();
        std::vector<Core::u8> nextPixels(static_cast<size_t>(nextW) * nextH * 4);

        for (Core::u32 y = 0; y < nextH; ++y) {
            const Core::u32 yStart = static_cast<Core::u32>(static_cast<Core::u64>(y) * currentH / nextH);
            const Core::u32 yEnd = static_cast<Core::u32>(static_cast<Core::u64>(y + 1) * currentH / nextH);
            for (Core::u32 x = 0; x < nextW; ++x) {
                const Core::u32 xStart = static_cast<Core::u32>(static_cast<Core::u64>(x) * currentW / nextW);
                const Core::u32 xEnd = static_cast<Core::u32>(static_cast<Core::u64>(x + 1) * currentW / nextW);
                const Core::u32 sampleCount = (xEnd - xStart) * (yEnd - yStart);
                for (Core::u32 channel = 0; channel < 4; ++channel) {
                    Core::u32 sum = 0;
                    for (Core::u32 srcY = yStart; srcY < yEnd; ++srcY) {
                        for (Core::u32 srcX = xStart; srcX < xEnd; ++srcX) {
                            sum += previous[(static_cast<size_t>(srcY) * currentW + srcX) * 4 + channel];
                        }
                    }
                    nextPixels[(static_cast<size_t>(y) * nextW + x) * 4 + channel] =
                        static_cast<Core::u8>(sum / std::max(1u, sampleCount));
                }
            }
        }

        mipData.push_back(std::move(nextPixels));
        currentW = nextW;
        currentH = nextH;
    }

    if (mipData.size() > header.mips.size()) {
        ENGINE_LOG_ERROR("AssetCooker: Generated mip count exceeds the binary format capacity.");
        return false;
    }
    header.mipCount = static_cast<Core::u32>(mipData.size());

    Core::u64 currentOffset = sizeof(TextureFileHeader);
    size_t totalBytes = sizeof(TextureFileHeader);
    currentW = width;
    currentH = height;
    for (size_t mipIndex = 0; mipIndex < mipData.size(); ++mipIndex) {
        auto& descriptor = header.mips[mipIndex];
        descriptor.fileOffset = currentOffset;
        descriptor.byteSize = mipData[mipIndex].size();
        descriptor.width = currentW;
        descriptor.height = currentH;
        descriptor.rowPitch = currentW * 4;

        if (!AddSize(totalBytes, mipData[mipIndex].size()) ||
            currentOffset > std::numeric_limits<Core::u64>::max() - mipData[mipIndex].size()) {
            ENGINE_LOG_ERROR("AssetCooker: Texture serialization size overflow.");
            return false;
        }
        currentOffset += mipData[mipIndex].size();
        currentW = std::max(1u, currentW / 2);
        currentH = std::max(1u, currentH / 2);
    }

    header.commonHeader.uncompressedSize = totalBytes;
    std::vector<Core::u8> fileBuffer(totalBytes);
    std::memcpy(fileBuffer.data(), &header, sizeof(header));
    Core::u8* cursor = fileBuffer.data() + sizeof(header);
    for (const auto& mip : mipData) {
        std::memcpy(cursor, mip.data(), mip.size());
        cursor += mip.size();
    }

    if (!VirtualFileSystem::Get().WriteFile(outVirtualPath, fileBuffer.data(), fileBuffer.size())) {
        ENGINE_LOG_ERROR("AssetCooker: Failed writing texture to '{}'.", outVirtualPath);
        return false;
    }

    AssetMetadata metadata{};
    metadata.guid = guid;
    metadata.cookedPath = outVirtualPath;
    metadata.assetType = "Texture";
    AssetDatabase::Get().RegisterAsset(metadata);

    ENGINE_LOG_INFO("AssetCooker: Cooked texture '{}' ({}x{}, {} MIPs, {:.2f} KB)",
        outVirtualPath, width, height, header.mipCount, totalBytes / 1024.0f);
    return true;
}

bool AssetCooker::CookMaterial(
    const MaterialFileHeader& materialData,
    const std::string& outVirtualPath,
    AssetGUID guid
) {
    MaterialFileHeader header = materialData;
    header.commonHeader.magic = CMAT_MAGIC;
    header.commonHeader.version = 1;
    header.commonHeader.guid = guid;
    header.commonHeader.uncompressedSize = sizeof(MaterialFileHeader);

    if (!VirtualFileSystem::Get().WriteFile(outVirtualPath, &header, sizeof(MaterialFileHeader))) {
        ENGINE_LOG_ERROR("AssetCooker: Failed writing material to '{}'.", outVirtualPath);
        return false;
    }

    AssetMetadata metadata{};
    metadata.guid = guid;
    metadata.cookedPath = outVirtualPath;
    metadata.assetType = "Material";
    if (materialData.albedoTextureGUID.IsValid()) metadata.dependencies.push_back(materialData.albedoTextureGUID);
    if (materialData.normalTextureGUID.IsValid()) metadata.dependencies.push_back(materialData.normalTextureGUID);
    if (materialData.metallicRoughnessTextureGUID.IsValid()) metadata.dependencies.push_back(materialData.metallicRoughnessTextureGUID);
    AssetDatabase::Get().RegisterAsset(metadata);

    ENGINE_LOG_INFO("AssetCooker: Cooked material '{}'", outVirtualPath);
    return true;
}

} // namespace Engine::Assets
