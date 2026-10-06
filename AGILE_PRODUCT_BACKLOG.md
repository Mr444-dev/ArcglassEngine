# Portfolio backlog

Priority is ordered around making the project easy to build, understand, and verify.

## P0 — evidence and reproducibility

- Record a repeatable editor and scene demo; commit the clip or screenshots.
- Keep clean Windows configure/build/test commands in the README and CI.
- Remove the remaining compiler warning.
- Register CTest cases per subsystem and identify GPU-dependent tests.

## P1 — useful interactive slice

- Add scene serialization and a sample scene asset.
- Connect one renderer pass to real scene geometry and document the frame path.
- Add a real asset-import path and a clear failure message for unsupported formats.
- Make camera and object controls discoverable in the editor UI.

## P2 — correctness and release hygiene

- Validate resource lifetime and synchronization with the D3D12 debug layer.
- Add automated validation for shaders and resource barriers.
- Decide whether Vulkan/Linux and deployment folders are active targets or archival experiments.
- Add a license and third-party notices before public release.

## Current boundaries

The project is a Windows Direct3D 12 prototype. The editor and runtime start, and the current test suite passes on the local GTX 1660 SUPER machine. Advanced renderer modules are not demonstrated as an integrated runtime pipeline. Do not present unverified performance figures as benchmarks.
