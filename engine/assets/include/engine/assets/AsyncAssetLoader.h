#pragma once

#include "engine/assets/BinaryFormats.h"
#include "engine/assets/VirtualFileSystem.h"
#include "engine/core/JobSystem.h"
#include <functional>
#include <memory>

namespace Engine::Assets {

struct LoadedMesh {
    MeshFileHeader header{};
    std::vector<SubmeshDescriptor> submeshes;
    std::vector<MeshletDescriptor> meshlets;
    std::vector<MeshVertex> vertices;
    std::vector<Core::u32> indices;
    bool isValid{false};
};

class AsyncAssetLoader {
public:
    static std::shared_ptr<LoadedMesh> LoadMesh(const std::string& virtualPath);

    static void LoadMeshAsync(
        const std::string& virtualPath,
        std::function<void(std::shared_ptr<LoadedMesh>)> onComplete
    );
};

} // namespace Engine::Assets
