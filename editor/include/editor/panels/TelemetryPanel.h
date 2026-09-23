#pragma once

#include "editor/EditorPanel.h"
#include <vector>
#include <string>

namespace Engine::Editor {

class TelemetryPanel : public EditorPanel {
public:
    TelemetryPanel();
    ~TelemetryPanel() override = default;

    void OnRender(EditorContext& context) override;

    void OnRenderDocCaptureClicked() noexcept;

    [[nodiscard]] const std::vector<std::string>& GetTelemetryLogs() const noexcept {
        return m_telemetryLogs;
    }

private:
    std::vector<std::string> m_telemetryLogs;
};

} // namespace Engine::Editor
