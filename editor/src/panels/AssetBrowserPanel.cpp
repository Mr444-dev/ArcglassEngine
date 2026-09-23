#include "editor/panels/AssetBrowserPanel.h"
#include "engine/assets/VirtualFileSystem.h"
#include <filesystem>
#include <algorithm>

namespace Engine::Editor {

AssetBrowserPanel::AssetBrowserPanel()
    : EditorPanel("Asset Browser") {
    Assets::VirtualFileSystem::Get().Mount("assets", "./assets");
    RefreshFiles("assets://");
}

void AssetBrowserPanel::RefreshFiles(const std::string& virtualFolder) {
    m_currentFolder = virtualFolder;
    m_files.clear();

    const std::string physical = Assets::VirtualFileSystem::Get().ResolvePhysicalPath(virtualFolder);

    if (std::filesystem::exists(physical) && std::filesystem::is_directory(physical)) {
        for (const auto& entry : std::filesystem::directory_iterator(physical)) {
            AssetFileItem item{};
            item.filename = entry.path().filename().string();
            item.virtualPath = virtualFolder + "/" + item.filename;
            item.isDirectory = entry.is_directory();
            item.extension = entry.path().extension().string();
            m_files.push_back(std::move(item));
        }
    } else {
        // Fallback default mock items if physical assets folder is empty
        m_files.push_back({"Sponza.cmesh", "assets://Sponza.cmesh", ".cmesh", false});
        m_files.push_back({"Bricks_Albedo.ctex", "assets://Bricks_Albedo.ctex", ".ctex", false});
        m_files.push_back({"Bricks_Normal.ctex", "assets://Bricks_Normal.ctex", ".ctex", false});
        m_files.push_back({"PBR_Bricks.cmat", "assets://PBR_Bricks.cmat", ".cmat", false});
    }

    std::sort(m_files.begin(), m_files.end(), [](const AssetFileItem& a, const AssetFileItem& b) {
        if (a.isDirectory != b.isDirectory) return a.isDirectory > b.isDirectory;
        return a.filename < b.filename;
    });
}

void AssetBrowserPanel::OnRender(EditorContext& context) {
    (void)context;
}

} // namespace Engine::Editor
