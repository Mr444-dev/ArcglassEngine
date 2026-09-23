#include "engine/assets/AssetCooker.h"
#include "engine/assets/VirtualFileSystem.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include <cstring>
#include <cmath>

namespace Engine::Assets {

std::vector<MeshletDescriptor> AssetCooker::GenerateMeshlets(
    const std::vector<MeshVertex>& vertices,
    const std::vector<Core::u32>& indices
) {
    std::vector<MeshletDescriptor> meshlets;
    if (indices.empty()) return meshlets;

    constexpr Core::u32 MAX_VERTICES_PER_MESHLET = 64;
    constexpr Core::u32 MAX_TRIANGLES_PER_MESHLET = 126;
    (void)MAX_VERTICES_PER_MESHLET;

    const size_t triangleCount = indices.size() / 3;
    meshlets.reserve(triangleCount / MAX_TRIANGLES_PER_MESHLET + 1);

    for (size_t t = 0; t < triangleCount; t += MAX_TRIANGLES_PER_MESHLET) {
        const size_t batchTris = std::min<size_t>(MAX_TRIANGLES_PER_MESHLET, triangleCount - t);

        MeshletDescriptor m{};
        m.triangleOffset = static_cast<Core::u32>(t * 3);
        m.triangleCount = static_cast<Core::u32>(batchTris);

        // Find min and max vertex indices in this triangle batch
        Core::u32 minV = 0xFFFFFFFF;
        Core::u32 maxV = 0;
        Core::Vec3 center{0.0f, 0.0f, 0.0f};

        for (size_t i = 0; i < batchTris * 3; ++i) {
            Core::u32 idx = indices[m.triangleOffset + i];
            minV = std::min(minV, idx);
            maxV = std::max(maxV, idx);
            center = center + vertices[idx].position;
        }

        const float invCount = 1.0f / static_cast<float>(batchTris * 3);
        center = center * invCount;

        // Calculate bounding sphere radius
        float maxRadiusSq = 0.0f;
        for (size_t i = 0; i < batchTris * 3; ++i) {
            Core::u32 idx = indices[m.triangleOffset + i];
            Core::Vec3 diff = vertices[idx].position - center;
            maxRadiusSq = std::max(maxRadiusSq, diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
        }

        m.vertexOffset = minV;
        m.vertexCount = (maxV >= minV) ? (maxV - minV + 1) : 0;
        m.boundingSphere[0] = center.x;
        m.boundingSphere[1] = center.y;
        m.boundingSphere[2] = center.z;
        m.boundingSphere[3] = std::sqrt(maxRadiusSq);

        meshlets.push_back(m);
    }

    return meshlets;
}

bool AssetCooker::CookMesh(
    const std::vector<MeshVertex>& vertices,
    const std::vector<Core::u32>& indices,
    const std::string& outVirtualPath,
    AssetGUID guid
) {
    if (vertices.empty() || indices.empty()) {
        ENGINE_LOG_ERROR("AssetCooker: Cannot cook empty mesh geometry");
        return false;
    }

    // 1. Compute bounding AABB
    Core::AABB totalBounds;
    totalBounds.min = vertices[0].position;
    totalBounds.max = vertices[0].position;
    for (const auto& v : vertices) {
        totalBounds.Encapsulate(Core::AABB{v.position, v.position});
    }

    // 2. Generate modern meshlets
    std::vector<MeshletDescriptor> meshlets = GenerateMeshlets(vertices, indices);

    // 3. Prepare Submesh
    SubmeshDescriptor submesh{};
    submesh.vertexOffset = 0;
    submesh.vertexCount = static_cast<Core::u32>(vertices.size());
    submesh.indexOffset = 0;
    submesh.indexCount = static_cast<Core::u32>(indices.size());
    submesh.meshletOffset = 0;
    submesh.meshletCount = static_cast<Core::u32>(meshlets.size());
    submesh.materialIndex = 0;
    submesh.aabb = totalBounds;

    // 4. Construct Header
    MeshFileHeader header{};
    header.commonHeader.magic = CMESH_MAGIC;
    header.commonHeader.version = 1;
    header.commonHeader.guid = guid;
    header.submeshCount = 1;
    header.totalVertexCount = static_cast<Core::u32>(vertices.size());
    header.totalIndexCount = static_cast<Core::u32>(indices.size());
    header.totalMeshletCount = static_cast<Core::u32>(meshlets.size());
    header.totalBounds = totalBounds;

    const size_t headerSize = sizeof(MeshFileHeader);
    const size_t submeshSize = sizeof(SubmeshDescriptor);
    const size_t meshletSize = sizeof(MeshletDescriptor) * meshlets.size();
    const size_t vertexSize = sizeof(MeshVertex) * vertices.size();
    const size_t indexSize = sizeof(Core::u32) * indices.size();
    const size_t totalBytes = headerSize + submeshSize + meshletSize + vertexSize + indexSize;

    header.commonHeader.uncompressedSize = totalBytes;

    // 5. Serialize into contiguous memory block
    std::vector<Core::u8> buffer(totalBytes);
    Core::u8* ptr = buffer.data();

    std::memcpy(ptr, &header, headerSize); ptr += headerSize;
    std::memcpy(ptr, &submesh, submeshSize); ptr += submeshSize;
    if (!meshlets.empty()) {
        std::memcpy(ptr, meshlets.data(), meshletSize); ptr += meshletSize;
    }
    std::memcpy(ptr, vertices.data(), vertexSize); ptr += vertexSize;
    std::memcpy(ptr, indices.data(), indexSize);

    // 6. Write through VFS
    if (!VirtualFileSystem::Get().WriteFile(outVirtualPath, buffer.data(), buffer.size())) {
        ENGINE_LOG_ERROR("AssetCooker: Failed writing cooked mesh to '{}'", outVirtualPath);
        return false;
    }

    // 7. Register in Database
    AssetMetadata meta{};
    meta.guid = guid;
    meta.cookedPath = outVirtualPath;
    meta.assetType = "Mesh";
    AssetDatabase::Get().RegisterAsset(meta);

    ENGINE_LOG_INFO("AssetCooker: Cooked mesh '{}' ({} verts, {} tris, {} meshlets, {:.2f} KB)",
        outVirtualPath, vertices.size(), indices.size() / 3, meshlets.size(), totalBytes / 1024.0f);
    return true;
}

bool AssetCooker::CookTexture(
    Core::u32 width,
    Core::u32 height,
    const Core::u8* rgbaPixels,
    const std::string& outVirtualPath,
    AssetGUID guid
) {
    if (width == 0 || height == 0 || !rgbaPixels) return false;

    TextureFileHeader header{};
    header.commonHeader.magic = CTEX_MAGIC;
    header.commonHeader.version = 1;
    header.commonHeader.guid = guid;
    header.width = width;
    header.height = height;
    header.depth = 1;
    header.format = TextureCompressionFormat::RGBA8_UNORM;

    // Generate MIP chain
    std::vector<std::vector<Core::u8>> mipData;
    Core::u32 currentW = width;
    Core::u32 currentH = height;

    // Mip 0 (original)
    const size_t mip0Size = width * height * 4;
    mipData.emplace_back(rgbaPixels, rgbaPixels + mip0Size);

    while (currentW > 1 || currentH > 1) {
        const Core::u32 nextW = std::max(1u, currentW / 2);
        const Core::u32 nextH = std::max(1u, currentH / 2);
        const auto& prevPixels = mipData.back();

        std::vector<Core::u8> nextPixels(nextW * nextH * 4);

        // 2x2 box filter
        for (Core::u32 y = 0; y < nextH; ++y) {
            for (Core::u32 x = 0; x < nextW; ++x) {
                const Core::u32 srcX0 = x * 2;
                const Core::u32 srcX1 = std::min(srcX0 + 1, currentW - 1);
                const Core::u32 srcY0 = y * 2;
                const Core::u32 srcY1 = std::min(srcY0 + 1, currentH - 1);

                for (int c = 0; c < 4; ++c) {
                    Core::u32 sum = 0;
                    sum += prevPixels[(srcY0 * currentW + srcX0) * 4 + c];
                    sum += prevPixels[(srcY0 * currentW + srcX1) * 4 + c];
                    sum += prevPixels[(srcY1 * currentW + srcX0) * 4 + c];
                    sum += prevPixels[(srcY1 * currentW + srcX1) * 4 + c];
                    nextPixels[(y * nextW + x) * 4 + c] = static_cast<Core::u8>(sum / 4);
                }
            }
        }

        mipData.push_back(std::move(nextPixels));
        currentW = nextW;
        currentH = nextH;
    }

    header.mipCount = static_cast<Core::u32>(mipData.size());

    // Calculate file offsets
    Core::u64 currentOffset = sizeof(TextureFileHeader);
    size_t totalBytes = sizeof(TextureFileHeader);
    currentW = width;
    currentH = height;

    for (size_t m = 0; m < mipData.size(); ++m) {
        header.mips[m].fileOffset = currentOffset;
        header.mips[m].byteSize = mipData[m].size();
        header.mips[m].width = currentW;
        header.mips[m].height = currentH;
        header.mips[m].rowPitch = currentW * 4;

        currentOffset += mipData[m].size();
        totalBytes += mipData[m].size();

        currentW = std::max(1u, currentW / 2);
        currentH = std::max(1u, currentH / 2);
    }

    header.commonHeader.uncompressedSize = totalBytes;

    // Serialize
    std::vector<Core::u8> fileBuffer(totalBytes);
    std::memcpy(fileBuffer.data(), &header, sizeof(TextureFileHeader));

    Core::u8* ptr = fileBuffer.data() + sizeof(TextureFileHeader);
    for (const auto& mip : mipData) {
        std::memcpy(ptr, mip.data(), mip.size());
        ptr += mip.size();
    }

    if (!VirtualFileSystem::Get().WriteFile(outVirtualPath, fileBuffer.data(), fileBuffer.size())) {
        ENGINE_LOG_ERROR("AssetCooker: Failed writing texture to '{}'", outVirtualPath);
        return false;
    }

    AssetMetadata meta{};
    meta.guid = guid;
    meta.cookedPath = outVirtualPath;
    meta.assetType = "Texture";
    AssetDatabase::Get().RegisterAsset(meta);

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
        ENGINE_LOG_ERROR("AssetCooker: Failed writing material to '{}'", outVirtualPath);
        return false;
    }

    AssetMetadata meta{};
    meta.guid = guid;
    meta.cookedPath = outVirtualPath;
    meta.assetType = "Material";
    if (materialData.albedoTextureGUID.IsValid()) meta.dependencies.push_back(materialData.albedoTextureGUID);
    if (materialData.normalTextureGUID.IsValid()) meta.dependencies.push_back(materialData.normalTextureGUID);
    if (materialData.metallicRoughnessTextureGUID.IsValid()) meta.dependencies.push_back(materialData.metallicRoughnessTextureGUID);
    AssetDatabase::Get().RegisterAsset(meta);

    ENGINE_LOG_INFO("AssetCooker: Cooked material '{}'", outVirtualPath);
    return true;
}

} // namespace Engine::Assets
