# Square128 modern runtime integration

Version4 profile: `luminance-30ppd-age24-D65-sp0-map14-square128`.
Fixed128x128 positive Float32 luminance,30ppd,age24,D65,sp0.
Separate18-payload package, FFT ABI256, MapGPU ABI14 and variable-count
mean/base adaptation. Previous runtime profiles retain their payloads.

Evidence root: `validation/base256_639286962762078797`.
Private full-quality acceptance: six pairs,414 checks/3042894 Float32/Float64
values bitwise against official v3.0.7, including the historical one-ULP
dominant-frequency near-tie. Private visual/SPYR:258 checks/15865410 Float32
words exact. These cohorts overlap.

## Public acceptance

The production API matches all69 checked stages on each of the same six pairs:
**414 checks/3042894 Float32/Float64 values bitwise**, including Q/Q_JOD,
masking, base-band selection, frequency, noise and adaptation. The CLI accepts
six original PFM pairs and preserves seven exports each against original input,
native reference and accepted public API bits. Quality text values and PFM
orientation/readback are checked separately. No reported Vulkan validation
errors in the accepted runs.

Five synthetic controls are asymmetric, impulse, HDR ramp, constant and
identity. The sixth pair reproduces the preserved historical `competing_axes`
PFMs exactly, with no gain, offset, gamma or resize applied. Its largest
distinct residual magnitudes are5018.5791015625 and5018.57861328125, one
Float32 ULP apart. The first native maximum is one-based index33; the GPU
selects zero-based32 and preserves the two tied maxima. Fresh native Q/Q_JOD
also match the historical pair. This is a stress control, not photograph/RGB
dataset acceptance.

## Component evidence and precision

| Check | Scope | Result |
| --- | --- | --- |
| Mean/DC, paired difference pack, residual FFT/magnitude/index | Six pairs/12 base bands,10770 Float32 words | Bitwise exact |
| SPYR from native P maps through base selection | Six pairs,66 checks/726546 Float32 words | Bitwise exact |
| GPU log mean/power10 | Six native log-adaptation bands,12 Float32 values | Bitwise exact |
| Actual GPU-bin frequency and NCSF noise | Six pairs,12 Float64 values | Bitwise exact |
| Original images through optical/retinal/P/log/SPYR/base | Six pairs,258 checks/15865410 Float32 words | Bitwise exact |
| Full private and public quality | Same six pairs,414 checks/3042894 values each | Bitwise exact |

These are overlapping views of one six-pair cohort. Initial five-pair runs are
retained separately with their original executables; they do not constitute
additional independent images. The identity case preserves zero frequency,
positive-infinite NCSF noise and Q/Q_JOD10. NaN is not accepted.

Reference: official HDR-VDP-3 v3.0.7,Octave11.3,CPU FFTW3 threads3,estimate
planning and fresh single wisdom. Stock/traced Q/Q_JOD agree before exports.
Inputs are positive Float32 luminance at30ppd/age24,D65/sp0. Full visual
padding is exactly256x256: symmetric post for MTF, replicated post for local
adaptation. Prepared/filtered FFT arrays are column-major complex;
forward/inverse traces are row-major. FFT256 uses forward graphs8/9, inverse
graphs3/4, once1/65536 normalization. No DC removal occurs during filtering.

The base FFT is unpadded16x16. Mean/DC results are row-major; paired test-minus-
reference is packed once into column-major complex input with +0 imaginary
components. The new residual ABI reuses the pinned FFTW real16/complex16
graphs, preserves native whole-matrix real narrowing and selects the first
column-major maximum after ignoring DC magnitude. Signed-zero bits are checked.
The generic mean and norm exponent-domain checks remain enforced.

Frequency and double NCSF evaluation remain on the CPU, using actual GPU
indices, adaptation and CSF parameters. The isolated limit check starts from
native resized/clamped log bands; the full visual and quality checks start
from original images and do not inject image-dependent native intermediates.

## Runtime contracts and regressions

V4 loads the explicit FFT256 graph flag, MapGPU ABI14 and both variable-count
flags. V1-v3 keep FFT3/4, map13 and their original count contracts. New asset
fields have defaults compatible with earlier source initializers; consumers
must rebuild. Binary struct-layout compatibility is not claimed.

Each of four runtime versions passes valid-package loading and12 malformed
package rejections without GPU work. Wrong geometry/profile pairings reject
in both directions. V4 rejects ppd60,age60 and spectral overrides before
exports;160square remains unsupported. The age60 check supplies the spectral
argument required by the CLI parser to reach the API guard.

Post-promotion old API regressions retain207 checks/721123 values across v1
rectangle,v2 real96 and v3 asymmetric80. Old v1/v2 CLI exports and the v2+128
profile rejection are preserved. The original four-shape general base/SPYR
diagnostics also pass again. Earlier broad legacy suites retain their own
historical acceptance and binary lineage; this report does not retroactively
claim signed-zero parity for them.

## Source reproduction

From the repository root, using a fresh, reasonably short output path:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_evaluation/generate_square128_runtime.ps1 -OutputDirectory validation/my_square128_runtime -VulkanBin C:/VulkanSDK/1.4.363.0/Bin
```

Use `validation/my_square128_runtime/runtime` with `--hdrvdp3-runtime`.
The generator reproduces common shaders/models, the already pinned six legacy
128 assets, and the general mean/difference/residual/base shaders. Its18
payloads are pinned by `REFERENCE_SQUARE128_PATHWAY_HASHES.json`. No reference
installation, image input or previous numerical export is needed to generate
the package.

Both `src` and isolated `iso` reproduce all18 payloads and manifest bytes
exactly. The isolated source tree contains660 source/dependency files, including
original AMD ASM/include sources; it excludes validation exports, images and
compiled shaders/libraries/executables. This proves shader/model regeneration,
not an isolated build of the full host API. The manifest SHA256 is
`f687079b87cbe3818bc9c80d033234c5063f007d5b73f6b09e4e6f010fd172e0`.

An initial residual shader compile wrapper expected local_size64 instead of
the preserved base16 graph's32; the first build stopped before numerical work.
The corrected wrapper and separate `fft_shader_v2` output are accepted. No
numerical mismatch required a tolerance or pin change in this checkpoint.

## Artifacts and remaining scope

Key directories: `acceptance_six`, `spyr_acceptance_six`,
`limit_acceptance_six`, `visual43_all`, `quality_acceptance`, `public_api_v4`,
`public_cli_v4`, `public_regression_v4`, `contracts_v1` through `contracts_v4`.
Fresh native traces and historical PFM hashes are retained beside them.
Source/binary/artifact inventories preserve the private/public cohort lineage.

This completes the fixed128square quality profile through modern public
API/CLI. It extends the earlier separate legacy128 integration; it does not
replace that historical evidence. RGB/color input, arbitrary dimensions,
general viewing conditions, other devices and additional prediction modes
still require implementation or acceptance. CPU model/LUT/NCSF and host
transfers remain; fully GPU-resident processing is not claimed.
