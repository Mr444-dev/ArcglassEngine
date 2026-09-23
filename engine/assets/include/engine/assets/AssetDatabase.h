#pragma once

#include "engine/assets/BinaryFormats.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <shared_mutex>

namespace Engine::Assets {

struct AssetMetadata {
    AssetGUID guid{};
    std::string virtualSourcePath;
    std::string cookedPath;
    std::string assetType;
    std::vector<AssetGUID> dependencies;
};

class AssetDatabase {
public:
    static AssetDatabase& Get();

    AssetDatabase() = default;
    ~AssetDatabase() = default;

    AssetDatabase(const AssetDatabase&) = delete;
    AssetDatabase& operator=(const AssetDatabase&) = delete;

    void RegisterAsset(const AssetMetadata& metadata);
    void UnregisterAsset(const AssetGUID& guid);

    [[nodiscard]] bool HasAsset(const AssetGUID& guid) const;
    [[nodiscard]] const AssetMetadata* GetMetadata(const AssetGUID& guid) const;
    [[nodiscard]] const AssetMetadata* GetMetadataByPath(const std::string& virtualPath) const;

    [[nodiscard]] size_t GetAssetCount() const;
    void Clear();

private:
    std::unordered_map<std::string, AssetMetadata> m_assetsByGuid;
    std::unordered_map<std::string, std::string> m_pathToGuid;
    mutable std::shared_mutex m_mutex;
};

} // namespace Engine::Assets
