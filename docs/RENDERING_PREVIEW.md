# Arcglass viewport rendering preview

The editor and runtime use a full-screen HLSL ray-cast preview. This is a deliberately small showcase path, separate from the engine's experimental RenderGraph modules.

## Directional light and object shading

The editor reads the normalized world-space position of the `Sun_DirectionalLight` scene marker as the direction from a surface toward the sun. The same vector drives the viewport sun disc and diffuse/specular object shading. A small ambient term keeps faces readable when turned away from the sun.

Scene objects cast approximate hard shadows by testing a ray against scaled bounding spheres around the other primitive entities. These bounds are intentionally inexpensive and do not match every primitive exactly. This is not the engine's Cascaded Shadow Maps implementation and does not provide filtered shadow maps or general multi-light support.

## Terrain preview

The editor evaluates an analytic height function directly in the viewport shader. The center remains flat for the starter scene; broad low-frequency hills fade in outside that area. Normals come from finite differences, and the preview blends grass/rock colors by slope and height. A short five-sample horizon check approximates terrain self-shadow. The editor grid and world axes fade as the height field rises.

The height field is not a terrain mesh or geometry clipmap. It is not fed to collision, navigation, or the runtime game's flat-ground simulation. The runtime sample keeps its flat gameplay floor to preserve its current jump and pickup logic.

## Sky and clouds

The shared shader include provides a horizon/zenith gradient, a solar disc and glow, plus a three-octave value-noise cloud layer projected onto a fixed altitude. The runtime animates the cloud coordinates; the editor preview uses a static time value. The cloud layer is a lightweight 2D shader effect, not volumetric cloud rendering. There is no atmospheric LUT, temporal reprojection, weather system, or physical atmosphere integration.

Published work on scalable atmosphere rendering and real-time cloudscapes informed the design constraints: [Hillaire, *A Scalable and Production Ready Sky and Atmosphere Rendering Technique*](https://diglib.eg.org/items/8a3e5350-18b3-46bd-9274-3add5af88c75) and [Guerrilla Games, *The Real-Time Volumetric Cloudscapes of Horizon: Zero Dawn*](https://www.guerrilla-games.com/read/the-real-time-volumetric-cloudscapes-of-horizon-zero-dawn). Arcglass does not implement those production techniques.

## Branding and type

`engine/platform/resources/Arcglass.ico` contains seven PNG-backed Windows icon sizes from 16 to 256 pixels and is compiled into both executables as a Win32 resource. The UI uses the bundled Inter font rasterized into a compact monochrome 8x12 glyph table, avoiding any font download or runtime installation dependency. It is still a bitmap UI font, not DirectWrite or a scalable font atlas. Regenerate the multi-size icon from its checked-in standard-library Python generator with `py -3 scripts/generate_app_icon.py` from the repository root.

## Build check

```cmd
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release --parallel
ctest --test-dir build/arcglass-msvc-release -C RelWithDebInfo --output-on-failure
```

Build requirements are listed in the main [README](../README.md).
