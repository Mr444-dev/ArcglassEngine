# Architecture

## Build graph

~~~mermaid
graph TD
    Core[Core] --> Platform[Win32 platform]
    Platform --> RHI[Direct3D 12 RHI]
    Core --> World[World and ECS]
    Core --> Assets[Assets and VFS]
    RHI --> Renderer[Renderer utilities]
    World --> Physics[Physics]
    World --> Animation[Animation]
    World --> Audio[Audio]
    Renderer --> Editor[Editor]
    World --> Editor
    Renderer --> Runtime[Runtime]
    World --> Runtime
    RHI --> Tests[Tests]
~~~

This is the CMake target dependency picture. It does not mean every module is used in the current runtime frame.

## Layers

- **Core:** base types, logging, assertions, allocators, job system, profiler.
- **Platform:** Win32 window lifecycle and input.
- **RHI:** Direct3D 12 device, command queue, descriptor heaps, swap chain, buffers, textures, and pipeline-state wrappers.
- **World:** entity/component registry, scene, transform hierarchy, and spatial BVH.
- **Assets:** virtual file system, binary asset helpers, cooking, and asynchronous loading.
- **Renderer:** render graph and rendering-related utilities, passes, HLSL experiments, and diagnostics.
- **Other subsystems:** physics, animation, audio, and networking modules.
- **Applications:** small editor and standalone interactive sample.
- **Tests:** one CTest executable containing subsystem and Direct3D 12 checks.

## Rendering scope

The Direct3D 12 device and swap-chain path is exercised by tests. The editor draws a custom HLSL interface and procedural starter scene. The runtime is a separate interactive sample.

GPU-driven meshlets, HZB occlusion, ray tracing, volumetrics, voxel GI, neural radiance cache, and virtual texturing are not demonstrated as a complete, integrated frame pipeline in the runtime. Treat their source and unit-level checks as experiments under development.

## Constraints

- Verified platform: Windows x64 with MSVC and Direct3D 12 feature level 12.0.
- The Windows build uses CMake and Ninja.
- Linux, Vulkan, container, and Kubernetes artifacts have not been verified.
- The editor has no production project/scene serialization or mature asset-import workflow.
- No source-level performance benchmark suite has been independently run for this audit.
