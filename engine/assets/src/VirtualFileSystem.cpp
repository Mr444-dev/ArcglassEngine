#include "engine/assets/VirtualFileSystem.h"
#include "engine/core/Log.h"
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <limits>
#include <mutex>

namespace Engine::Assets {
namespace {

bool IsContainedPath(const std::filesystem::path& root, const std::filesystem::path& candidate) {
    const std::filesystem::path relative = candidate.lexically_relative(root);
    if (relative.empty()) {
        return candidate == root;
    }

    if (relative.is_absolute() || relative.has_root_name() || relative.has_root_directory()) {
        return false;
    }

    for (const auto& component : relative) {
        if (component == "..") {
            return false;
        }
    }
    return true;
}

} // namespace

VirtualFileSystem& VirtualFileSystem::Get() {
    static VirtualFileSystem instance;
    return instance;
}

void VirtualFileSystem::Mount(const std::string& virtualPrefix, const std::string& physicalRoot) {
    if (virtualPrefix.empty() || virtualPrefix.find("://") != std::string::npos ||
        virtualPrefix.find('/') != std::string::npos || virtualPrefix.find('\\') != std::string::npos) {
        ENGINE_LOG_ERROR("VFS: Refusing invalid mount prefix '{}'.", virtualPrefix);
        return;
    }

    if (physicalRoot.empty()) {
        ENGINE_LOG_ERROR("VFS: Refusing an empty physical mount root.");
        return;
    }

    std::error_code error;
    const auto absoluteRoot = std::filesystem::absolute(physicalRoot, error);
    if (error) {
        ENGINE_LOG_ERROR("VFS: Failed to make mount root '{}' absolute ({}).", physicalRoot, error.message());
        return;
    }
    const auto root = std::filesystem::weakly_canonical(absoluteRoot, error);
    if (error || root.empty()) {
        ENGINE_LOG_ERROR("VFS: Failed to canonicalize mount root '{}' ({}).", physicalRoot, error.message());
        return;
    }

    std::unique_lock lock(m_mutex);
    m_mountPoints[virtualPrefix] = root.generic_string();
    ENGINE_LOG_INFO("VFS: Mounted '{}://' -> '{}'", virtualPrefix, m_mountPoints[virtualPrefix]);
}

void VirtualFileSystem::Unmount(const std::string& virtualPrefix) {
    std::unique_lock lock(m_mutex);
    m_mountPoints.erase(virtualPrefix);
}

std::string VirtualFileSystem::ResolvePhysicalPath(const std::string& virtualPath) const {
    const auto schemePosition = virtualPath.find("://");
    if (schemePosition == std::string::npos) {
        // Preserve explicit local paths used for shader and platform files.
        return virtualPath;
    }

    const std::string prefix = virtualPath.substr(0, schemePosition);
    const std::string relativePath = virtualPath.substr(schemePosition + 3);

    std::filesystem::path mountRoot;
    {
        std::shared_lock lock(m_mutex);
        const auto mount = m_mountPoints.find(prefix);
        if (mount == m_mountPoints.end()) {
            ENGINE_LOG_WARN("VFS: Unknown mount prefix '{}' in path '{}'.", prefix, virtualPath);
            return {};
        }
        mountRoot = std::filesystem::path(mount->second);
    }

    const std::filesystem::path requested(relativePath);
    if (requested.has_root_name() || requested.has_root_directory()) {
        ENGINE_LOG_WARN("VFS: Refusing rooted path '{}' under mount '{}://'.", relativePath, prefix);
        return {};
    }
    for (const auto& component : requested) {
        if (component == "..") {
            ENGINE_LOG_WARN("VFS: Refusing parent traversal in virtual path '{}'.", virtualPath);
            return {};
        }
    }

    std::error_code error;
    const auto canonicalRoot = std::filesystem::weakly_canonical(mountRoot, error);
    if (error) {
        ENGINE_LOG_ERROR("VFS: Failed to resolve mount root '{}' ({}).", mountRoot.generic_string(), error.message());
        return {};
    }

    const auto canonicalCandidate = std::filesystem::weakly_canonical(mountRoot / requested, error);
    if (error || !IsContainedPath(canonicalRoot, canonicalCandidate)) {
        ENGINE_LOG_WARN("VFS: Resolved path escapes mount '{}' or cannot be canonicalized: '{}'.", prefix, virtualPath);
        return {};
    }

    return canonicalCandidate.generic_string();
}

bool VirtualFileSystem::FileExists(const std::string& virtualPath) const {
    const std::string physical = ResolvePhysicalPath(virtualPath);
    if (physical.empty()) return false;

    std::error_code error;
    return std::filesystem::is_regular_file(physical, error) && !error;
}

bool VirtualFileSystem::ReadFile(const std::string& virtualPath, std::vector<Core::u8>& outData) const {
    outData.clear();
    const std::string physical = ResolvePhysicalPath(virtualPath);
    if (physical.empty()) return false;

    std::ifstream file(physical, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        ENGINE_LOG_ERROR("VFS: Failed to open file for read: '{}' ({})", virtualPath, physical);
        return false;
    }

    const std::streampos end = file.tellg();
    if (end == std::streampos(-1)) {
        ENGINE_LOG_ERROR("VFS: Failed to determine file size for '{}'.", physical);
        return false;
    }
    const auto fileSize = static_cast<std::uintmax_t>(static_cast<std::streamoff>(end));
    if (fileSize > std::numeric_limits<size_t>::max() ||
        fileSize > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max())) {
        ENGINE_LOG_ERROR("VFS: File is too large to read: '{}'.", physical);
        return false;
    }

    outData.resize(static_cast<size_t>(fileSize));
    file.seekg(0, std::ios::beg);
    if (fileSize > 0 && !file.read(reinterpret_cast<char*>(outData.data()), static_cast<std::streamsize>(fileSize))) {
        ENGINE_LOG_ERROR("VFS: Failed reading bytes from '{}'.", physical);
        outData.clear();
        return false;
    }
    return true;
}

bool VirtualFileSystem::WriteFile(const std::string& virtualPath, const void* data, size_t size) {
    if ((data == nullptr && size != 0) ||
        size > static_cast<size_t>(std::numeric_limits<std::streamsize>::max())) {
        ENGINE_LOG_ERROR("VFS: Refusing invalid write request for '{}'.", virtualPath);
        return false;
    }

    const std::string physical = ResolvePhysicalPath(virtualPath);
    if (physical.empty()) return false;

    std::error_code error;
    const std::filesystem::path path(physical);
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path(), error);
        if (error) {
            ENGINE_LOG_ERROR("VFS: Failed creating parent directory for '{}' ({}).", physical, error.message());
            return false;
        }
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        ENGINE_LOG_ERROR("VFS: Failed to open file for write: '{}' ({})", virtualPath, physical);
        return false;
    }

    if (size > 0) {
        file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    }
    return file.good();
}

} // namespace Engine::Assets
