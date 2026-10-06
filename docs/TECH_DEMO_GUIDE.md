# Editor demo and recording guide

This guide describes current prototype behavior, not planned or unintegrated renderer features.

## Start

Build from a Visual Studio Developer Command Prompt:

~~~cmd
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release --parallel
~~~

Launch `build\arcglass-msvc-release\bin\EngineEditor.exe` or run `run_editor.bat` from the repository root. CMake compiles `shaders/editor/EditorUI.hlsl` and `shaders/geometry/RuntimeScene.hlsl` with the Windows SDK `fxc.exe`, then places their `.cso` files beside the matching executable. Neither app compiles these shaders on its UI thread at launch, and loading the files does not depend on the current working directory.

The editor and standalone runtime were launched from the fresh build on the local Windows/Direct3D 12 development machine (GTX 1660 SUPER). The editor source creates a custom shader-rendered UI and a procedural starter scene. It previews the directional sun, analytic terrain, approximate shadows, and sky/clouds in its ray-cast viewport; these are described precisely in [RENDERING_PREVIEW.md](RENDERING_PREVIEW.md). The runtime is a small interactive Crystal Seeker sample, not a complete game.

## Controls

- Select scene rows in the left hierarchy.
- Q selects; W moves; E rotates; R scales.
- Drag in the central viewport to manipulate the selected entity. Hold Shift while dragging to adjust its height.
- Hold right mouse and use WASD to fly the camera; use Q/E for vertical movement.
- Hold Alt and drag with the left mouse button to orbit the selected entity.
- Press Space to enter play mode; press it again to restore the pre-play scene state.
- Press F5 to toggle debug drawing.
- Press Esc to close a menu or exit.

The interface is a custom HLSL canvas with an embedded 8x12 Inter bitmap font and a small generated vector-style brand mark. It is not a mature docking editor; some panels are visual prototypes and do not provide the workflow depth of a commercial engine.

## Suggested 45-second capture

1. **0–8 s:** Start on the editor window and show the layout and starter scene.
2. **8–18 s:** Select the sphere and cylinder in the hierarchy.
3. **18–28 s:** Switch to move mode and drag one object; show its inspector values updating.
4. **28–36 s:** Enter play mode with Space, then exit and show the scene restoring.
5. **36–45 s:** Toggle debug drawing with F5, move the camera briefly, and end on the editor layout.

Use OBS or another screen recorder to capture the application window at 1080p and 30 fps. Keep the clip focused on repeatable actions. Do not add benchmark overlays or claim advanced renderer effects until they are demonstrated by the runtime and measured.

No demo recording is currently committed in the repository.
