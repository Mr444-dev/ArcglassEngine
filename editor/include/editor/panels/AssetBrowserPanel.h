#pragma once

#include "editor/EditorPanel.h"
#include <vector>
#include <string>

namespace Engine::Editor {

struct AssetFileItem {
    std::string filename;
    std::string virtualPath;
    std::string extension;
    bool isDirectory{false};
};

class AssetBrowserPanel : public EditorPanel {
public:
    AssetBrowserPanel();
    ~AssetBrowserPanel() override = default;

    void OnRender(EditorContext& context) override;

    void RefreshFiles(const std::string& virtualFolder = "assets://");
    [[nodiscard]] const std::vector<AssetFileItem>& GetFiles() const noexcept { return m_files; }

    void SelectFile(const std::string& virtualPath) noexcept { m_selectedPath = virtualPath; }
    [[nodiscard]] const std::string& GetSelectedPath() const noexcept { return m_selectedPath; }

private:
    std::string m_currentFolder{"assets://"};
    std::string m_selectedPath;
    std::vector<AssetFileItem> m_files;
};

} // namespace Engine::Editor
