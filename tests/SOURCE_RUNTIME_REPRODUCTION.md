# Source-only pathway runtime reproduction

Run from the repository root with Windows PowerShell,CMake/MSVC2026 and the
validated Vulkan SDK compiler tools:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_evaluation/generate_pathway_runtime.ps1 -OutputDirectory validation/my_source_runtime -VulkanBin C:/VulkanSDK/1.4.363.0/Bin
```

The ready package is `validation/my_source_runtime/runtime`. Use a new output
directory each run. The driver builds only its required standalone shaders and
CPU model exporter;it never invokes compile_shaders.sh or removes old outputs.
It reads tracked FFTW/AMD source tables and model coefficients,not previous
validation exports,compiled shaders,images,native DLLs or an official reference
installation. Normal CMake/compiler/Vulkan SDK installations are prerequisites.

Sixteen SPIR-V payloads are generated,including both mixed FFT orientations and
the accepted explicit Float32-control variants. The160x192 forward transform
retains its validated glslangValidator path;other compilers/flags are recorded.
`create_sp0_model` generates380 Float32 words directly from default production
coefficient literals with the required transpose. The CPU exporter writes the
four canonical word tables without loading any model fixture. The factory is
also compiled into the production/bounded libraries.

The20 payload files must match REFERENCE_PATHWAY_HASHES.json exactly before a
PASS marker is written. The driver does not accept a numerical tolerance or
silently update pins for a different compiler. A failure retains generated
sources/logs for diagnosis. Matching payloads imply reuse of the already tested
profile;this is reproduction coverage,not new image/device acceptance.

Checkpoint: validation/source_runtime_639285297000073643. Both the working-tree
build and an isolated source copy with no validation folder or compiled payloads
reproduce all20 SHA256 hashes and the complete pathway.runtime manifest byte
for byte. The first isolated-copy setup omitted tracked AMD.asm tables;that
failed attempt remains and is not a reproduction pass. The completed copy
includes those source tables. Initial normal compilerstderr handling was also
corrected;exit codes now determine failures and logs are retained.

The generated package passes12 loader mutation checks. A source-generated
package run from an unrelated working directory reproduces accepted HDR96x80
input/P/log/score/quality bytes with no Vulkan errors. This overlaps prior
content coverage. Exact profile remains30ppd,age24,D65,sp0,80x96/96x80 on the
recorded Intel Arc Pro device;CPU NCSF and synchronous host transfers remain.
