#pragma once

#include "editor/EditorPanel.h"
#include <vector>
#include <string>

namespace Engine::Editor {

class PropertiesPanel : public EditorPanel {
public:
    PropertiesPanel();
    ~PropertiesPanel() override = default;

    void OnRender(EditorContext& context) override;

    template <typename T, typename... Args>
    T& AddComponentToSelected(EditorContext& context, Args&&... args) {
        ENGINE_ASSERT(context.HasSelection(), "No entity selected");
        return context.scene->GetRegistry().Emplace<T>(context.selectedEntity, std::forward<Args>(args)...);
    }

    template <typename T>
    void RemoveComponentFromSelected(EditorContext& context) {
        ENGINE_ASSERT(context.HasSelection(), "No entity selected");
        context.scene->GetRegistry().Remove<T>(context.selectedEntity);
    }

    [[nodiscard]] const std::vector<std::string>& GetInspectorLogs() const noexcept { return m_inspectorLogs; }

private:
    void InspectComponents(World::EntityID entity, EditorContext& context);

    std::vector<std::string> m_inspectorLogs;
};

} // namespace Engine::Editor
