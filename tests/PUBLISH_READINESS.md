# Selected-source publish preparation

2026-10-23 evidence: `validation/publish_prepare_639283319336040304`.
The proposed manifest contains102 selected paths,including this document.
No index staging,commit or push was performed during preparation.

The clean source snapshot combines committed HEAD
`688eff7085661c8f3427434e12c37b081e6f3ca7` with only the selected working files.
VkFFT comes from its recorded commit
`539be29f30ef40384f3ccd24d7884166f82cbef8`,not its dirty working directory.
This is an exported source snapshot,not a newly committed Git worktree.

- Normal independent API/application targets configure and build successfully.
- All19 runtime shaders across bounded/128/rectangular packages regenerate
  from snapshot sources with pinned-identical hashes.
- Both callers preserve620 numerical export hashes across32square and both
  rectangular orientations,including quality files;API score bits match.
- All35 public invalid-input cases reject correctly.

These are fresh export comparisons to earlier strict native baselines,not new
independent native oracle executions. An initial PowerShell launch stopped on
informational Vulkan stderr due to ErrorActionPreference;its log is retained
and excluded. Only `*_verified_*` runs enter smoke_results.json.

This checkpoint prepares the bounded HDR-VDP-3 library, source generators,
tests, application luminance entry and validation documentation for review.
It excludes local numerical artifacts, compiled binaries, the independently
staged `hdrvdp3_corrdn_test.glsl`, dirty VkFFT working files, local glslang
copies and root build experiments. Existing files and index entries are retained.

The supported dependency-isolated build uses the existing application sources
through an optional target. It requires CMake3.29+,a C++20 compiler,the Vulkan
SDK and the recorded VkFFT submodule headers:

```powershell
git submodule update --init lib/VkFFT
cmake -S tests/hdrvdp3_evaluation -B build-hdrvdp3 -DIQM_HDRVDP3_BUILD_APPLICATION=ON
cmake --build build-hdrvdp3 --config Release --target IQM-quality-api-test IQM-hdrvdp3
```

`IQM-hdrvdp3` uses the same main,argument parser and HDR-VDP-3 wrapper as the
normal application,with other metrics disabled. The independent CMake project
generates its own seven embedded shader headers and links SDK compiler libraries
directly. It does not execute the VkFFT benchmark CMake project,modify the
submodule or require `shaders_out` from the developer workspace. The optional
application target is OFF by default,so the existing direct API build is retained.

Runtime packages regenerate from repository sources using
`generate_assets.ps1` (bounded32/64),`generate_assets128.ps1` and
`generate_assets_rectangular.ps1`. Each requires a fresh output directory and
`-VulkanBin <SDK/Bin>` and enforces pinned binary hashes. No numerical reference
exports are inputs to shader generation.

This preparation validates the independent HDR-VDP-3 build,not the full root
multi-metric application build. Root `bin/CMakeLists.txt` experiments and local
dependency workarounds remain outside the proposed change. Library source CMake
integration is included because it lists the required runtime modules.

Historical validation documents and stress replay scripts refer to local
`validation/` checkpoints. Those datasets and binaries are intentionally not
published as source. Native oracle scripts require an external official
HDR-VDP-3 v3.0.7 installation and Octave packages;their native dependency paths
are documented in those scripts. The bounded runtime can use the existing
age24 constants or caller-supplied spectral samples;external reference images
and spectral files are separate from the clean build check.
