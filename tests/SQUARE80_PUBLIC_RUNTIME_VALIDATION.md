# Square80 runtime integration

Version3 profile: `luminance-30ppd-age24-D65-sp0-map13-square80`.
Fixed80x80 positive Float32 luminance,30ppd,age24,D65,sp0.
Separate18-payload package:14 shaders and4 model tables. Other geometries
are rejected by this profile; version1/version2 packages retain their scope.

Local evidence: `validation/square80_visual_639286716205199707`.
Private full-quality acceptance: five synthetic pairs,345 checks/990565
Float32/Float64 values match official v3.0.7 exactly. Visual/SPYR chain:
215 checks/5164555 Float32 words exact. These cohorts overlap.

## Completed acceptance

Public API: the same five pairs pass69 checks/198113 values each,
345 checks/990565 Float32/Float64 values total. Q/Q_JOD, all bands,
masking, base selection/noise and adaptation agree bitwise with official
HDR-VDP-3 v3.0.7. Public CLI: five PFM pairs, seven exports each, match
original input, native and accepted public API bits. Quality text values are
also checked after Float32 conversion; PFM orientation/readback is exact.
No reported Vulkan validation errors in accepted runs.

The visual/SPYR acceptance starts from the original images, not reference
intermediates. It checks model coefficients, prepared FFT input, forward,
filtered and inverse arrays, retinal response, L+M preparation, adaptation,
pupil, P/log maps, all SPYR bands and base selection. Five synthetic contents
are asymmetric, impulse, HDR ramp, constant and identity. This is one shared
five-pair cohort across overlapping checks; it is not photograph/RGB acceptance.

Native configuration: v3.0.7,Octave11.3,CPU FFTW3 threads3,estimate planning,
fresh single wisdom,exact160x160 padding for80x80 inputs. MTF uses symmetric
post padding, local adaptation uses replicated post padding. Filters are
prepared from double model expressions and narrowed once to Float32.
Prepared/filtered complex arrays are column-major, forward/inverse traces
row-major. Entirely real complex results retain the native narrowing rule;
FFT/filtering does not remove DC. The residual base FFT is unpadded10x10,
with DC ignored only for dominant-bin selection.

New normalization divides by25600=25*1024 with one RTE rounding. Integer
normalization passes1054696 finite IEEE probes and204800 actual native FFTW
inverse components bitwise, including signed zeros and subnormal boundaries.
Reciprocal multiplication is not substituted for division.

## Runtime and compatibility

Version3's18 payloads are pinned in
`tests/hdrvdp3_evaluation/REFERENCE_SQUARE80_PATHWAY_HASHES.json`. The loader
sets explicit variable-mean/base count ABI flags for this profile only.
Version1/version2 retain their old counts, layouts and payload hashes. These
asset structures have new defaulted fields; consumers must rebuild from source.
The compatibility claim concerns accepted source callers/runtime packages,
not an unchanged binary structure layout.

Checks recorded under `contracts_v1`, `contracts_v2`, `contracts_v3` each
accept a valid packet and reject12 malformed variants before GPU work.
Wrong profile/geometry pairings reject: v3 with96square or rectangles,
and v1/v2 with80square. V3 rejects ppd60,age60 and spectral overrides before
exports. The age60 API-guard check supplies the spectral argument required by
the CLI parser; its absence is separately rejected at parsing.

`api_regression` preserves69 checks each for both old v1 tiny-contrast
orientations and the v2 real96 case. `production_regression_v3` preserves
seven exports for v1/v2 and modern128 rejection. Earlier broad suites retain
their historical binaries and are not attributed to the new executable.

During initial public integration, variable mean count was mistakenly sent
as the height for subsequent residual stages. Strict validation caught the
incorrect base-band D values; the failed `public_api` output is preserved.
The fix limits that count to the mean stage. `public_api_final` is the
accepted run; no tolerance or shader pin was relaxed. The first inverse160
candidate also retained an old192 output stride; its failed run and shader
remain in place, while `inverse_shader_v2` is the accepted correction.

## Source-only reproduction

From the repository root, with the documented Windows CMake/MSVC/Vulkan SDK:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_evaluation/generate_square80_runtime.ps1 -OutputDirectory validation/my_square80_runtime -VulkanBin C:/VulkanSDK/1.4.363.0/Bin
```

Use the resulting `validation/my_square80_runtime/runtime` with
`--hdrvdp3-runtime`. Output directories must be fresh. The generator rebuilds
the scalar/SIMD FFTW-derived graphs, IO, general base operations, common
shaders and source-model tables; it does not require reference numerical
exports or an installed native reference.

`source_build` and isolated `iso` reproduce all18 payload hashes and manifest
bytes exactly. Manifest SHA256:
`9f0176cf7c9e62718c22e6a214a641d53ba1b13836ead8a84dd26b7b2e7dd388`.
Isolation includes source dependencies and original AMD ASM files; no images,
compiled shaders/libraries/executables or validation exports are inputs.
This proves shader/model package regeneration; the public host API is tested
separately above. Earlier isolated attempts/logs remain: omitted ASM dependency,
then an MSBuild long-path failure. The successful run uses the shorter `iso`
output path. Use a reasonably short fresh build path on Windows.

## Run the locally tested inputs

From the repository root in PowerShell:

```powershell
& './validation/square_public_639286351443539038/build/Release/IQM-hdrvdp3.exe' --method HDRVDP3 --input './validation/square80_visual_639286716205199707/public_cli/asymmetric_test.pfm' --ref './validation/square80_visual_639286716205199707/public_cli/asymmetric_ref.pfm' --hdrvdp3-runtime './validation/square80_visual_639286716205199707/runtime'
```

The CLI creates a fresh export directory automatically. These are local test
artifacts, excluded from Git. Other accepted pairs and their score records
are retained beside them in `public_cli`.

## Remaining metric scope

This completes the fixed80x80 luminance profile through public API/CLI.
Arbitrary dimensions, RGB/color inputs, general viewing conditions and broader
device coverage remain outside acceptance. CPU model/LUT generation, double
NCSF and synchronous host transfers remain; this is not fully GPU-resident
processing. Only Q/Q_JOD quality prediction is covered by these acceptance
results; other HDR-VDP-3 prediction modes require separate implementation
and validation.
