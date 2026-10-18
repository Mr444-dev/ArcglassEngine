#include "engine/assets/AsyncAssetLoader.h"
#include "engine/assets/VirtualFileSystem.h"
#include "engine/core/Log.h"
#include <cmath>
#include <cstring>
#include <memory>

namespace Engine::Assets {
namespace {

template <typename T>
bool ReadArray(
    const std::vector<Core::u8>& bytes,
    size_t& cursor,
    Core::u32 count,
    std::vector<T>& output
) {
    if (cursor > bytes.size() || count > (bytes.size() - cursor) / sizeof(T)) return false;
    output.resize(count);
    const size_t byteCount = static_cast<size_t>(count) * sizeof(T);
    if (byteCount > 0) {
        std::memcpy(output.data(), bytes.data() + cursor, byteCount);
    }
    cursor += byteCount;
    return true;
}

bool IsRangeValid(Core::u32 offset, Core::u32 count, Core::u32 total) noexcept {
    return offset <= total && count <= total - offset;
}

bool IsFinite(const Core::Vec3& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool IsValidBounds(const Core::AABB& bounds) noexcept {
    return IsFinite(bounds.min) && IsFinite(bounds.max) &&
        bounds.min.x <= bounds.max.x && bounds.min.y <= bounds.max.y && bounds.min.z <= bounds.max.z;
}

struct AsyncMeshTask {
    std::string virtualPath;
    std::function<void(std::shared_ptr<LoadedMesh>)> onComplete;
};

} // namespace

std::shared_ptr<LoadedMesh> AsyncAssetLoader::LoadMesh(const std::string& virtualPath) {
    auto mesh = std::make_shared<LoadedMesh>();
    const auto invalid = [&]() {
        ENGINE_LOG_ERROR("AsyncAssetLoader: Invalid or unsupported .cmesh file '{}'.", virtualPath);
        return mesh;
    };

    std::vector<Core::u8> fileBytes;
    if (!VirtualFileSystem::Get().ReadFile(virtualPath, fileBytes)) {
        ENGINE_LOG_ERROR("AsyncAssetLoader: Failed to read mesh file '{}'.", virtualPath);
        return mesh;
    }
    if (fileBytes.size() < sizeof(MeshFileHeader)) return invalid();

    MeshFileHeader header{};
    std::memcpy(&header, fileBytes.data(), sizeof(header));
    if (header.commonHeader.magic != CMESH_MAGIC || header.commonHeader.version != 1 ||
        header.commonHeader.compressedSize != 0 || header.commonHeader.uncompressedSize != fileBytes.size() ||
        header.submeshCount == 0 || header.totalVertexCount == 0 || header.totalIndexCount == 0 ||
        header.totalIndexCount % 3 != 0 || !IsValidBounds(header.totalBounds)) {
        return invalid();
    }

    size_t cursor = sizeof(MeshFileHeader);
    if (!ReadArray(fileBytes, cursor, header.submeshCount, mesh->submeshes) ||
        !ReadArray(fileBytes, cursor, header.totalMeshletCount, mesh->meshlets) ||
        !ReadArray(fileBytes, cursor, header.totalVertexCount, mesh->vertices) ||
        !ReadArray(fileBytes, cursor, header.totalIndexCount, mesh->indices) ||
        cursor != fileBytes.size()) {
        mesh->submeshes.clear();
        mesh->meshlets.clear();
        mesh->vertices.clear();
        mesh->indices.clear();
        return invalid();
    }

    for (const auto& vertex : mesh->vertices) {
        if (!IsFinite(vertex.position) || !IsFinite(vertex.normal) ||
            !std::isfinite(vertex.tangent[0]) || !std::isfinite(vertex.tangent[1]) ||
            !std::isfinite(vertex.tangent[2]) || !std::isfinite(vertex.tangent[3]) ||
            !std::isfinite(vertex.uv[0]) || !std::isfinite(vertex.uv[1])) {
            return invalid();
        }
    }
    for (const Core::u32 index : mesh->indices) {
        if (index >= mesh->vertices.size()) return invalid();
    }

    for (const auto& submesh : mesh->submeshes) {
        if (submesh.vertexCount == 0 || submesh.indexCount == 0 || submesh.indexCount % 3 != 0 ||
            !IsRangeValid(submesh.vertexOffset, submesh.vertexCount, header.totalVertexCount) ||
            !IsRangeValid(submesh.indexOffset, submesh.indexCount, header.totalIndexCount) ||
            !IsRangeValid(submesh.meshletOffset, submesh.meshletCount, header.totalMeshletCount) ||
            !IsValidBounds(submesh.aabb)) {
            return invalid();
        }

        const Core::u64 vertexEnd = static_cast<Core::u64>(submesh.vertexOffset) + submesh.vertexCount;
        const Core::u64 indexEnd = static_cast<Core::u64>(submesh.indexOffset) + submesh.indexCount;
        for (Core::u64 i = submesh.indexOffset; i < indexEnd; ++i) {
            const Core::u32 index = mesh->indices[static_cast<size_t>(i)];
            if (index < submesh.vertexOffset || index >= vertexEnd) return invalid();
        }

        const Core::u64 meshletEnd = static_cast<Core::u64>(submesh.meshletOffset) + submesh.meshletCount;
        for (Core::u64 i = submesh.meshletOffset; i < meshletEnd; ++i) {
            const auto& meshlet = mesh->meshlets[static_cast<size_t>(i)];
            if (meshlet.vertexCount == 0 || meshlet.vertexCount > 64 ||
                meshlet.triangleCount == 0 || meshlet.triangleCount > 126 ||
                !IsRangeValid(meshlet.vertexOffset, meshlet.vertexCount, header.totalVertexCount) ||
                !IsRangeValid(meshlet.triangleOffset, meshlet.triangleCount * 3, header.totalIndexCount) ||
                !std::isfinite(meshlet.boundingSphere[0]) || !std::isfinite(meshlet.boundingSphere[1]) ||
                !std::isfinite(meshlet.boundingSphere[2]) || !std::isfinite(meshlet.boundingSphere[3]) ||
                meshlet.boundingSphere[3] < 0.0f) {
                return invalid();
            }

            const Core::u64 meshletVertexEnd = static_cast<Core::u64>(meshlet.vertexOffset) + meshlet.vertexCount;
            const Core::u64 meshletIndexBegin = meshlet.triangleOffset;
            const Core::u64 meshletIndexEnd = meshletIndexBegin + static_cast<Core::u64>(meshlet.triangleCount) * 3;
            for (Core::u64 indexPosition = meshletIndexBegin; indexPosition < meshletIndexEnd; ++indexPosition) {
                const Core::u32 index = mesh->indices[static_cast<size_t>(indexPosition)];
                if (index < meshlet.vertexOffset || index >= meshletVertexEnd) return invalid();
            }
        }
    }

    mesh->header = header;
    mesh->isValid = true;
    ENGINE_LOG_INFO("AsyncAssetLoader: Successfully loaded '{}' ({} verts, {} meshlets)",
        virtualPath, mesh->vertices.size(), mesh->meshlets.size());
    return mesh;
}

void AsyncAssetLoader::LoadMeshAsync(
    const std::string& virtualPath,
    std::function<void(std::shared_ptr<LoadedMesh>)> onComplete
) {
    auto task = std::make_unique<AsyncMeshTask>(AsyncMeshTask{virtualPath, std::move(onComplete)});
    const bool submitted = Core::JobSystem::Get().SubmitJob(
        [](void* data) {
            std::unique_ptr<AsyncMeshTask> current(static_cast<AsyncMeshTask*>(data));
            auto loaded = LoadMesh(current->virtualPath);
            if (current->onComplete) current->onComplete(std::move(loaded));
        },
        task.get()
    );

    if (submitted) {
        task.release();
        return;
    }

    // Preserve completion semantics when the scheduler is not initialized or is shutting down.
    auto loaded = LoadMesh(task->virtualPath);
    if (task->onComplete) task->onComplete(std::move(loaded));
}

} // namespace Engine::Assets
