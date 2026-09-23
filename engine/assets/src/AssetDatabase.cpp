#include "engine/assets/AssetDatabase.h"
#include "engine/core/Log.h"

namespace Engine::Assets {

AssetDatabase& AssetDatabase::Get() {
    static AssetDatabase instance;
    return instance;
}

void AssetDatabase::RegisterAsset(const AssetMetadata& metadata) {
    std::unique_lock lock(m_mutex);
    const std::string guidStr = metadata.guid.ToString();

    m_assetsByGuid[guidStr] = metadata;
    if (!metadata.virtualSourcePath.empty()) {
        m_pathToGuid[metadata.virtualSourcePath] = guidStr;
    }
    if (!metadata.cookedPath.empty()) {
        m_pathToGuid[metadata.cookedPath] = guidStr;
    }

    ENGINE_LOG_INFO("AssetDatabase: Registered [{}] '{}' (GUID: {})", 
        metadata.assetType, metadata.virtualSourcePath, guidStr);
}

void AssetDatabase::UnregisterAsset(const AssetGUID& guid) {
    std::unique_lock lock(m_mutex);
    const std::string guidStr = guid.ToString();

    auto it = m_assetsByGuid.find(guidStr);
    if (it != m_assetsByGuid.end()) {
        m_pathToGuid.erase(it->second.virtualSourcePath);
        m_pathToGuid.erase(it->second.cookedPath);
        m_assetsByGuid.erase(it);
    }
}

bool AssetDatabase::HasAsset(const AssetGUID& guid) const {
    std::shared_lock lock(m_mutex);
    return m_assetsByGuid.find(guid.ToString()) != m_assetsByGuid.end();
}

const AssetMetadata* AssetDatabase::GetMetadata(const AssetGUID& guid) const {
    std::shared_lock lock(m_mutex);
    auto it = m_assetsByGuid.find(guid.ToString());
    return it != m_assetsByGuid.end() ? &it->second : nullptr;
}

const AssetMetadata* AssetDatabase::GetMetadataByPath(const std::string& virtualPath) const {
    std::shared_lock lock(m_mutex);
    auto itPath = m_pathToGuid.find(virtualPath);
    if (itPath != m_pathToGuid.end()) {
        auto itAsset = m_assetsByGuid.find(itPath->second);
        if (itAsset != m_assetsByGuid.end()) {
            return &itAsset->second;
        }
    }
    return nullptr;
}

size_t AssetDatabase::GetAssetCount() const {
    std::shared_lock lock(m_mutex);
    return m_assetsByGuid.size();
}

void AssetDatabase::Clear() {
    std::unique_lock lock(m_mutex);
    m_assetsByGuid.clear();
    m_pathToGuid.clear();
}

} // namespace Engine::Assets
