#include "engine/assets/VirtualFileSystem.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include <fstream>
#include <filesystem>

namespace Engine::Assets {

VirtualFileSystem& VirtualFileSystem::Get() {
    static VirtualFileSystem instance;
    return instance;
}

void VirtualFileSystem::Mount(const std::string& virtualPrefix, const std::string& physicalRoot) {
    std::unique_lock lock(m_mutex);
    std::filesystem::path p(physicalRoot);
    m_mountPoints[virtualPrefix] = std::filesystem::absolute(p).generic_string();
    ENGINE_LOG_INFO("VFS: Mounted '{}://' -> '{}'", virtualPrefix, m_mountPoints[virtualPrefix]);
}

void VirtualFileSystem::Unmount(const std::string& virtualPrefix) {
    std::unique_lock lock(m_mutex);
    m_mountPoints.erase(virtualPrefix);
}

std::string VirtualFileSystem::ResolvePhysicalPath(const std::string& virtualPath) const {
    std::shared_lock lock(m_mutex);

    const auto pos = virtualPath.find("://");
    if (pos == std::string::npos) {
        return virtualPath; // Local relative or absolute path
    }

    const std::string prefix = virtualPath.substr(0, pos);
    const std::string relativePath = virtualPath.substr(pos + 3);

    auto it = m_mountPoints.find(prefix);
    if (it != m_mountPoints.end()) {
        std::filesystem::path physical = std::filesystem::path(it->second) / relativePath;
        return physical.generic_string();
    }

    ENGINE_LOG_WARN("VFS: Unknown mount prefix '{}' in path '{}'", prefix, virtualPath);
    return relativePath;
}

bool VirtualFileSystem::FileExists(const std::string& virtualPath) const {
    const std::string physical = ResolvePhysicalPath(virtualPath);
    return std::filesystem::exists(physical) && std::filesystem::is_regular_file(physical);
}

bool VirtualFileSystem::ReadFile(const std::string& virtualPath, std::vector<Core::u8>& outData) const {
    const std::string physical = ResolvePhysicalPath(virtualPath);

    std::ifstream file(physical, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        ENGINE_LOG_ERROR("VFS: Failed to open file for read: '{}' ({})", virtualPath, physical);
        return false;
    }

    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    outData.resize(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(outData.data()), size)) {
        ENGINE_LOG_ERROR("VFS: Failed reading bytes from '{}'", physical);
        outData.clear();
        return false;
    }

    return true;
}

bool VirtualFileSystem::WriteFile(const std::string& virtualPath, const void* data, size_t size) {
    const std::string physical = ResolvePhysicalPath(virtualPath);

    std::filesystem::path p(physical);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }

    std::ofstream file(physical, std::ios::binary);
    if (!file.is_open()) {
        ENGINE_LOG_ERROR("VFS: Failed to open file for write: '{}' ({})", virtualPath, physical);
        return false;
    }

    file.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
    return file.good();
}

} // namespace Engine::Assets
