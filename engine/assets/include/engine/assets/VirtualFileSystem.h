#pragma once

#include "engine/core/BaseTypes.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <shared_mutex>

namespace Engine::Assets {

class VirtualFileSystem {
public:
    static VirtualFileSystem& Get();

    VirtualFileSystem() = default;
    ~VirtualFileSystem() = default;

    VirtualFileSystem(const VirtualFileSystem&) = delete;
    VirtualFileSystem& operator=(const VirtualFileSystem&) = delete;

    void Mount(const std::string& virtualPrefix, const std::string& physicalRoot);
    void Unmount(const std::string& virtualPrefix);

    // Returns an empty string when a virtual mount is unknown or a path escapes it.
    // Unqualified paths remain explicit local paths for engine-owned files.
    [[nodiscard]] std::string ResolvePhysicalPath(const std::string& virtualPath) const;
    [[nodiscard]] bool FileExists(const std::string& virtualPath) const;

    bool ReadFile(const std::string& virtualPath, std::vector<Core::u8>& outData) const;
    bool WriteFile(const std::string& virtualPath, const void* data, size_t size);

private:
    std::unordered_map<std::string, std::string> m_mountPoints;
    mutable std::shared_mutex m_mutex;
};

} // namespace Engine::Assets
