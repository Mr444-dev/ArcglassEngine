#pragma once

#include "engine/assets/BinaryFormats.h"
#include "engine/assets/AssetDatabase.h"
#include <cstddef>
#include <vector>
#include <string>

namespace Engine::Assets {

class AssetCooker {
public:
    static bool CookMesh(
        const std::vector<MeshVertex>& vertices,
        const std::vector<Core::u32>& indices,
        const std::string& outVirtualPath,
        AssetGUID guid = AssetGUID::CreateRandom()
    );

    static bool CookTexture(
        Core::u32 width,
        Core::u32 height,
        const Core::u8* rgbaPixels,
        size_t rgbaByteCount,
        const std::string& outVirtualPath,
        AssetGUID guid = AssetGUID::CreateRandom()
    );

    static bool CookMaterial(
        const MaterialFileHeader& materialData,
        const std::string& outVirtualPath,
        AssetGUID guid = AssetGUID::CreateRandom()
    );

private:
    static std::vector<MeshletDescriptor> GenerateMeshlets(
        const std::vector<MeshVertex>& vertices,
        const std::vector<Core::u32>& indices
    );
};

} // namespace Engine::Assets
