#pragma once

#include "editor/EditorContext.h"
#include <string>

namespace Engine::Editor {

class EditorPanel {
public:
    explicit EditorPanel(std::string name) : m_name(std::move(name)) {}
    virtual ~EditorPanel() = default;

    virtual void OnRender(EditorContext& context) = 0;

    [[nodiscard]] const std::string& GetName() const noexcept { return m_name; }
    [[nodiscard]] bool IsVisible() const noexcept { return m_isOpen; }
    void SetVisible(bool visible) noexcept { m_isOpen = visible; }

protected:
    std::string m_name;
    bool m_isOpen{true};
};

} // namespace Engine::Editor
