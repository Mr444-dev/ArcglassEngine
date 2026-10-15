<p align="center">
  <img src="docs/assets/portfolio-mark.svg" alt="Arcglass Engine logo" width="96" height="96">
</p>

<h1 align="center">Arcglass Engine</h1>

**Stack:** C++20 · CMake · MSVC · Win32 · Direct3D 12 · HLSL · CTest

Arcglass Engine is a Windows game-engine prototype written in C++20. It contains a Direct3D 12 rendering interface, engine subsystems, a small custom editor, a standalone runtime, and a native test executable.

This repository is a learning and portfolio project, not a production-ready engine. The editor and runtime start on the verified Windows development machine. Many engine modules have unit-level checks, but they are not all connected into a complete end-to-end game pipeline.

[![Windows CI](https://github.com/Mr444-dev/ArcglassEngine/actions/workflows/ci.yml/badge.svg)](https://github.com/Mr444-dev/ArcglassEngine/actions/workflows/ci.yml)


<p align="center"><a href="#why-this-project-exists">Overview</a> &nbsp;·&nbsp; <a href="#architecture">Architecture</a> &nbsp;·&nbsp; <a href="#build-requirements">Build</a> &nbsp;·&nbsp; <a href="#build-and-test">Tests</a> &nbsp;·&nbsp; <a href="#portfolio-status">Status</a></p>
## Why this project exists

A game engine needs clear boundaries between platform services, rendering, world data, assets, and tools. This project explores those boundaries in native C++ and keeps the verified Windows graphics path distinct from modules that remain experiments.

## Verified on this revision

Checks run from the renamed `ArcglassEngine` checkout on 2026-10-06:

- A fresh Windows x64 MSVC 19.51 configure and build completed for `EngineEditor`, `GameRuntime`, and `Engine_Tests`.
- Windows SDK `fxc.exe` compiled both the editor and runtime shader-model 5.1 programs; the Win32 resource compiler embedded the generated Arcglass `.ico` in both executables.
- CTest passed its one registered target (`Engine_UnitTests`).
- The editor viewport now has an editable directional-sun vector, approximate scene-object shadows, a procedural height-field preview, a sun disc, and a procedural cloud layer. These effects are shader previews, not integrated production render passes.
- The editor includes a filesystem-backed `Assets` panel with folder shortcuts, paging, refresh, and `Add Files` copy into the project asset tree. The UI and its shader compile; live interaction was not rechecked in this build.
- The editor/runtime executables and their shader outputs were generated in the fresh build directory. The icons are embedded in the executables; there are no runtime icon-file dependencies.
- Earlier local graphics checks used an NVIDIA GeForce GTX 1660 SUPER with about 6 GB dedicated VRAM. Other adapters have not been tested.

The checkout does not track executables. Fresh binaries are generated under the ignored `build/arcglass-msvc-release/bin/` directory. Historical binaries at the repository root are stale local artifacts and must not be published. CI rebuilds from source; no binary release is claimed.
## Engineering Highlights

- C++20 CMake project split into core, platform, RHI, world, asset, renderer, physics, animation, audio, networking, runtime, and editor targets.
- Win32 window and input layer.
- Direct3D 12 device, descriptor heaps, command lists, textures, swap chain, and shader compilation.
- Core allocators, job system, logging, and profiling utilities.
- Entity/component world, transform hierarchy, spatial BVH, asset virtual file system, and mesh/texture cooking.
- CPU-side physics, animation, audio, render-graph, streaming, and renderer utilities with unit-level checks.
- A small editor with a custom HLSL-rendered interface, an embedded Inter bitmap atlas with proportional advances, generated Arcglass brand mark, selectable scene objects, transform controls, scene files, and a filesystem-backed asset browser. Its ray-cast viewport previews a procedural sky/cloud layer, directional lighting, approximate shadows, and an analytic height field; see [rendering preview details](docs/RENDERING_PREVIEW.md).
- A standalone interactive runtime sample.

## Current limitations

- This is a prototype; there is no claim of production readiness, stable public API, or production game.
- The editor remains an early scene-editing prototype. Its interface is a custom full-screen HLSL canvas, not a mature docking UI; there is no project/workspace format, format-specific asset importer, thumbnail preview, material editor, or finished content pipeline. The asset browser lists files and copies selected local files into `assets/`; it does not validate, cook, or convert their contents. The terrain and lighting effects are viewport approximations rather than mesh-based RenderGraph features. Scene documents use the small `.arcscene` format; legacy `.mgescene` files remain loadable.
- The runtime is a graphics/input sample, not a complete game.
- Several renderer features exist as isolated C++ modules, shaders, or CPU-side tests. The runtime does not demonstrate the GPU-driven meshlet/HZB, ray-tracing, neural-radiance-cache, volumetric, or virtual-texture pipeline end to end.
- The DirectStorage manager is a CPU-only queue-lifecycle simulator. It does not read files, call DirectStorage, decompress GDeflate, write VRAM, or measure bandwidth. `shaders/compute/GDeflateDecompress.hlsl` is an unused experimental scaffold, not a working decoder.
- Vulkan headers and Linux/deployment files are exploratory scaffolding; only the Windows Direct3D 12 build and test path has been verified here.
- Performance values in historical planning documents have not been reproduced as benchmarks and are not presented as current results.
- One CTest test is registered; it invokes a larger executable containing multiple internal checks. This is not complete subsystem coverage.
- No license file is present. Add a license you choose before publishing publicly.

## My Contribution

This checkout contains the engine libraries, Direct3D 12 path, editor and runtime entry points, shader sources, subsystem checks, CMake configuration, and technical documentation. The README identifies which pieces are smoke-tested and which are not integrated into a complete game pipeline.

## Architecture

~~~mermaid
graph TD
    Core[Core: allocators, jobs, logging] --> Platform[Win32 platform and input]
    Platform --> RHI[Direct3D 12 RHI]
    RHI --> Renderer[Renderer utilities and render graph]
    Core --> World[Entity/component world]
    Core --> Assets[VFS and asset cooking]
    World --> Physics[Physics]
    World --> Animation[Animation]
    World --> Audio[Audio]
    Renderer --> Editor[Editor executable]
    World --> Editor
    Renderer --> Runtime[Runtime executable]
    World --> Runtime
    Core --> Tests[Engine test executable]
    RHI --> Tests
    World --> Tests
~~~

The diagram shows build dependencies, not a promise that every subsystem participates in each rendered frame.

## Build requirements

- Windows x64.
- Visual Studio C++ build tools with MSVC and a Windows SDK.
- CMake 3.28 or newer.
- Ninja.
- A Direct3D 12 capable GPU and driver exposing feature level 12.0 for the graphics test and editor.
- Windows SDK `fxc.exe` (used to precompile the editor and runtime shaders during the CMake build).

The verified local run used an NVIDIA GeForce GTX 1660 SUPER. Other adapters have not been tested. The repository does not currently provide a verified Linux/Vulkan build.

## Build and test

Run from a Visual Studio Developer Command Prompt (x64), or after initializing the matching Visual Studio environment:

~~~cmd
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release --parallel
ctest --test-dir build/arcglass-msvc-release -C RelWithDebInfo --output-on-failure
~~~

Launch the built editor or runtime:

~~~cmd
.\build\arcglass-msvc-release\bin\EngineEditor.exe
.\build\arcglass-msvc-release\bin\GameRuntime.exe
~~~

From the repository root, `run_editor.bat` and `run_tests.bat` use the same `build/arcglass-msvc-release/` preset output as the commands above. The build script initializes the latest Visual Studio installation through `vswhere` when it is not already running in a developer shell.

## Editor controls

- Q, W, E, R: select, move, rotate, and scale modes.
- Ctrl+S / Ctrl+O: save or open a scene (`.arcscene`).
- Ctrl+Z / Ctrl+Y: undo or redo scene changes.
- Delete / Backspace: remove the selected object.
- Space: enter or leave play mode.
- Hold right mouse and use WASD; use Q/E to move vertically.
- Alt + left mouse: orbit the selected entity.
- F5 or the toolbar button: toggle debug collider drawing.
- Esc: close a menu or exit.

These controls are implemented in the editor loop. The viewport and scene are a small interaction demo, not a finished authoring workflow.

## Code Tour

- `engine/` — core, platform, RHI, world, asset, rendering, and subsystem modules.
- `editor/` — custom editor executable and viewport interaction.
- `runtime/` — standalone graphics/input sample.
- `cmake/EngineShaders.cmake` — Windows SDK discovery and build-time HLSL bytecode targets used by both applications.
- `shaders/` — HLSL sources.
- `tests/` — native subsystem and graphics checks.
- `docs/` — architecture notes and demo walkthrough.

## Performance

No reproducible engine benchmark is published. The observed five-frame Direct3D 12 smoke run verifies that a window presented frames on one local GPU; it is not a throughput or frame-time benchmark.

## Repository layout

- engine/: engine libraries and renderer utilities.
- editor/: editor library and executable.
- runtime/: standalone runtime sample.
- shaders/: HLSL source files.
- tests/: internal unit and subsystem checks.
- docs/: architecture notes and an honest demo/recording walkthrough.
- .github/workflows/ci.yml: Windows configure, build, and CTest workflow.

## Key Systems

### Direct3D 12 renderer

The Windows rendering path creates a native device and swap chain, then presents frames through the engine renderer. The recorded local check exercised device creation and five frames on an NVIDIA GeForce GTX 1660 SUPER with about 6 GB of VRAM. This confirms one development machine, not broad GPU or driver compatibility.

### Editor and standalone runtime

The custom editor and standalone runtime are separate applications over shared engine code. This makes tool startup and runtime behavior inspectable independently, while the current modules are not yet connected into a complete authoring-to-playable-game pipeline.

### Native subsystem tests

Subsystem checks are collected in the native test executable and CTest target. The current CTest registration runs one test target; its internal checks include graphics initialization, but it is not a comprehensive end-to-end game suite.

## Design Decisions and Trade-offs

- The verified target is Windows x64 with Direct3D 12; exploratory Vulkan/Linux files are not presented as supported builds.
- The editor uses a small custom HLSL-rendered UI rather than claiming a full Dear ImGui authoring workflow.
- Subsystems are separated for inspection and testing, but several do not participate in a complete rendered game loop.
- A small standalone sample keeps the renderer path demonstrable while content pipelines and project persistence remain future work.

## Portfolio status

The strongest verified material is the C++20 module structure, Direct3D 12 device/swap-chain path, engine-side data structures, and repeatable Windows CI. Present renderer features as experiments or work in progress until they are rendered by the runtime and captured in a reproducible demo.


## What This Project Demonstrates

This repository provides concrete examples of:

- Native C++20 application structure across platform, renderer, engine subsystems, and tools.
- Windows graphics programming with Direct3D 12, HLSL, CMake, and MSVC.
- Separating verified runtime/editor behavior from incomplete engine integration and hardware-specific evidence.

## Author

**Michał Rusinek** · [GitHub](https://github.com/Mr444-dev) · [Portfolio](https://mr444-dev.github.io/)
