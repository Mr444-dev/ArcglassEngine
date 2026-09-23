#pragma once

#include "editor/EditorContext.h"
#include "engine/core/MathTypes.h"
#include "engine/renderer/Camera.h"

namespace Engine::Editor {

enum class GizmoAxis : Core::u8 {
    None = 0,
    X = 1,
    Y = 2,
    Z = 3,
    XYZ = 4
};

class GizmoController {
public:
    GizmoController() = default;
    ~GizmoController() = default;

    static void ApplyTranslation(EditorContext& context, const Core::Vec3& delta);
    static void ApplyRotation(EditorContext& context, const Core::Vec3& deltaEuler);
    static void ApplyScale(EditorContext& context, const Core::Vec3& deltaScale);

    static Core::Vec3 CalculateTranslationDelta(
        GizmoAxis axis,
        float mouseDeltaX,
        float mouseDeltaY,
        const Renderer::Camera& camera
    );
};

} // namespace Engine::Editor
