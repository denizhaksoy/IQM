# RGB through pupil and JND

2026-10-27 checkpoint: `validation/rgb_response_639287164909180665`.
Native captures: path in `validation/rgb_jnd_active_path.txt`, `native/`.

The original-input RGB96 chain now computes optical filtering, independently
calibrated spectral projection, L+M, local adaptation, pupil reduction, L/M/rod
JND and achromatic P. Three display spectra × two images pass **884,742
Float32 bit comparisons**: earlier 497,664 optical/adaptation words plus six
pupil scalars and 387,072 post-pupil/JND/P words. These cohorts overlap earlier
results; they are not additional image pairs. Native version3.0.7, Octave11.3,
30ppd,age24, Float32 input, default quality parameters and native symmetric
MTF/replicate adaptation padding remain fixed. No tolerance relaxation.

`MapGPUProfile::RGBResponse` is explicit ABI17. `jnd_response_lmsr` requires
that ABI and accepts four column-major LMSR planes before response clamp.
It selects L, M and rod (plane4), clamps, applies pupil reduction, and uses
the existing exact logarithm/product/interpolation arithmetic. It does not
apply the mono D65 matrix. S does not enter achromatic JND, as in the native
code. CPU LUT generation and host-vector transfers remain.

`generate_rgb_jnd_shader.ps1` creates a new isolated shader from the existing
source generator, adds mode37 and marks ABI17. Old ABI13–16 shader files and
runtime packets are unchanged. The module declares Float32 denormal preserve
and round-to-nearest-even controls and passes spirv-val. Intel Arc Pro runs
have Vulkan validation enabled; no validation errors were reported.

The checker uses actual GPU-derived row-major adaptation for pupil reduction;
the scalar agrees with the native value cast to Float32. It checks the three
post-pupil L/M/rod planes, three JND planes and P against native captures.
The old public v4 `competing_axes` luminance regression still passes 69 checks /
507,149 values bitwise after the new API/profile addition.

## Reproduction

Build `rgb-optical-gpu-test` from `tests/hdrvdp3_residual144`. Generate a fresh
ABI17 module:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/generate_rgb_jnd_shader.ps1 -OutputDirectory validation/rgb_map_new -VulkanBin C:/VulkanSDK/1.4.363.0/Bin
```

Checker arguments in order: native `--jnd` captures, public v2 runtime/assets,
original spectral data directory, accepted RGB projection SPIR-V, fresh output,
accepted RGB adaptation SPIR-V, new ABI17 map SPIR-V. Full invocation and paths
are recorded by the handoff checkpoint and active pointers.

Initial acceptance used a stale PASS description saying JND was not accepted;
its numerical comparisons did include JND/P. `acceptance_final` reruns the
same arithmetic with the corrected scope text. An initial PowerShell launch
was rejected by local execution policy before running; explicit process-local
`-ExecutionPolicy Bypass` succeeded without changing persistent policy.

## Remaining work and delivery priority

This does not yet compute final RGB Q/Q_JOD. Compose the accepted P/adaptation
with SPYR and quality, implementing the native comparison mask as the OR of
per-channel relative differences (`any(...,3)`). Zero denominators need native
NaN/Inf comparison semantics; collapsing RGB to a grayscale mask is incorrect.
Then validate complete scores/intermediates and integrate explicit RGB public
inputs/runtime. The user's delivery limit is two weeks: prioritize an exact,
usable release at validated geometries before broadening size/viewing/mode
support. No universal reference-parity or fully GPU-resident claim.
