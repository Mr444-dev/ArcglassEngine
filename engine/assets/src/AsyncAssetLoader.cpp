#include "engine/assets/AsyncAssetLoader.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include <cstring>

namespace Engine::Assets {

std::shared_ptr<LoadedMesh> AsyncAssetLoader::LoadMesh(const std::string& virtualPath) {
    auto mesh = std::make_shared<LoadedMesh>();

    std::vector<Core::u8> fileBytes;
    if (!VirtualFileSystem::Get().ReadFile(virtualPath, fileBytes)) {
        ENGINE_LOG_ERROR("AsyncAssetLoader: Failed to read mesh file: '{}'", virtualPath);
        return mesh;
    }

    if (fileBytes.size() < sizeof(MeshFileHeader)) {
        ENGINE_LOG_ERROR("AsyncAssetLoader: Mesh file corrupted (too small): '{}'", virtualPath);
        return mesh;
    }

    const Core::u8* ptr = fileBytes.data();
    std::memcpy(&mesh->header, ptr, sizeof(MeshFileHeader));
    ptr += sizeof(MeshFileHeader);

    // Defensive header verification
    if (mesh->header.commonHeader.magic != CMESH_MAGIC) {
        ENGINE_LOG_ERROR("AsyncAssetLoader: Invalid magic (expected 0x{:08X}, got 0x{:08X}) in '{}'",
            CMESH_MAGIC, mesh->header.commonHeader.magic, virtualPath);
        return mesh;
    }

    // Read submeshes
    mesh->submeshes.resize(mesh->header.submeshCount);
    std::memcpy(mesh->submeshes.data(), ptr, sizeof(SubmeshDescriptor) * mesh->header.submeshCount);
    ptr += sizeof(SubmeshDescriptor) * mesh->header.submeshCount;

    // Read meshlets
    if (mesh->header.totalMeshletCount > 0) {
        mesh->meshlets.resize(mesh->header.totalMeshletCount);
        std::memcpy(mesh->meshlets.data(), ptr, sizeof(MeshletDescriptor) * mesh->header.totalMeshletCount);
        ptr += sizeof(MeshletDescriptor) * mesh->header.totalMeshletCount;
    }

    // Read vertices
    mesh->vertices.resize(mesh->header.totalVertexCount);
    std::memcpy(mesh->vertices.data(), ptr, sizeof(MeshVertex) * mesh->header.totalVertexCount);
    ptr += sizeof(MeshVertex) * mesh->header.totalVertexCount;

    // Read indices
    mesh->indices.resize(mesh->header.totalIndexCount);
    std::memcpy(mesh->indices.data(), ptr, sizeof(Core::u32) * mesh->header.totalIndexCount);

    mesh->isValid = true;
    ENGINE_LOG_INFO("AsyncAssetLoader: Successfully loaded '{}' ({} verts, {} meshlets)",
        virtualPath, mesh->vertices.size(), mesh->meshlets.size());
    return mesh;
}

struct AsyncMeshTask {
    std::string virtualPath;
    std::function<void(std::shared_ptr<LoadedMesh>)> onComplete;
};

void AsyncAssetLoader::LoadMeshAsync(
    const std::string& virtualPath,
    std::function<void(std::shared_ptr<LoadedMesh>)> onComplete
) {
    auto* task = new AsyncMeshTask{virtualPath, std::move(onComplete)};

    Core::JobSystem::Get().SubmitJob(
        [](void* data) {
            auto* t = static_cast<AsyncMeshTask*>(data);
            auto loaded = LoadMesh(t->virtualPath);
            if (t->onComplete) {
                t->onComplete(std::move(loaded));
            }
            delete t;
        },
        task
    );
}

} // namespace Engine::Assets
