# General SPYR and base-band pipeline checkpoint

Checkpoint: 2026-10-27. Evidence root:
`validation/base_general_639286707604688716`.

This extends the [variable-size mean/DC stage](GENERAL_BASE_MEAN_VALIDATION.md)
with general paired difference packing, small residual FFT/magnitude/selection
and variable-count base adaptation. It adds an isolated 80x80 geometry. It
does **not** add 80x80 to the public runtime or establish arbitrary-size support.

## Native configuration and input equivalence

Official HDR-VDP-3 v3.0.7, CPU Octave11.3/FFTW3, three FFTW threads, estimate
planning, fresh single wisdom before each captured FFT; Float32 positive
luminance,30ppd,age24,D65,sp0. Five contents: asymmetric, impulse, HDR ramp,
constant and identity. Nonidentity uses the documented localized gain1.4 and
offset0.25. New80x80 and fresh rectangular captures assert unmodified official
Q/Q_JOD equals the instrumented copy exactly. Existing96x96 references remain
unchanged and are reused. These are synthetic controls, not new HDR photographs.

Full-image padding remains exactly2H x2W in the native visual pathway. The
isolated residual FFT is **unpadded**, operating on the native base difference.
Mean/DC inputs are row-major original base bands. Packing subtracts centered
test-reference once and writes column-major complex input with +0 imaginary
parts. FFT output is row-major complex; magnitude ignores DC and selects the
first column-major maximum. Entirely real complex results receive the native
whole-matrix imaginary +0 narrowing. FFT zeros are compared by bits.

## Measured results

| Scope | Cases | Comparisons | Result |
| --- | --- | --- | --- |
| Three-stage mean/DC, difference pack, FFT/magnitude/index | 20 pairs across10x10,10x12,12x10,12x12 bases | 17,000 Float32 words | Bitwise exact |
| SPYR from original native P maps through base selection | Same20 pairs | 220 checks /1,144,720 Float32 words | Bitwise exact |
| Variable-count GPU log mean/power10 | Same20 pairs | 40 Float32 words | Bitwise exact |
| Frequency and NCSF noise from actual GPU-selected bin | Same20 pairs | 40 Float64 values | Bitwise exact |
| Full quality with three alternate base shaders | Existing six96x96 pairs | 414 checks /1,711,662 Float32/Float64 values | Bitwise exact |
| Original144 pipeline, unchanged accepted shaders | Existing five96x96 pairs | All recorded stage checks | Bitwise exact |

These scopes overlap; do not sum them as independent image acceptance.
The four source geometries are80x80,80x96,96x80,96x96. Eighty-square is the
new geometry; the other three provide controls. The80-square SPYR comparison
starts from **native P maps**, and the adaptation comparison starts from the
**native resized/clamped log-adaptation band**. It does not validate the new
geometry's preceding GPU optical filtering, retinal response or resize path.

New80x80 nonidentity cases select zero-based index10, frequency
0.37499999999999989. Identity selects index0,frequency0 and positive-infinite
noise in all four geometries, matching the reference. `metadata.json` encodes
infinity as null plus an explicit positive-infinity-limit flag; exact native
frequency/noise values are separately retained as two little-endian Float64
values per case. They are checked without a tolerance.

Frequency and double NCSF evaluation still run on the CPU, using GPU-selected
indices and GPU adaptation/CSF parameters. They have not been moved to Vulkan.
Host transfers remain. No reported Vulkan validation errors in accepted runs.

## Implementation and ABI boundaries

- `mean_general_difference_pack.comp`: dimensions in push words2/3, paired
  mean+centered groups, up to64x64 base dimensions. Numerical acceptance here
  covers only the four listed bases, not every size within that guard.
- `generate_residual_general_extended_shader.ps1`: reuses accepted FFTW
  real10/12 and complex10/12 graphs, allowing only10/12 on each axis. Scratch
  space supports at most144 values. It is not a general FFT library.
- `generate_base_general_shader.ps1`: uses the accepted general mean with
  native-style power10; push words are groups,count,reserved,reserved. Caller
  enforces finite log values in[-5,5] and the mean exponent-domain contract.
- `IQM_HDRVDP3_GENERAL_SPYR_DIAGNOSTIC` allows the four source geometries
  only in the separately compiled diagnostic. It uses the historical private
  SPYR symbol alias plus the general mean push ABI. Default production guards,
  version1/version2 manifests and pinned payloads are unchanged.
- Full-quality regression uses the previously frozen
  `mean_general_639286695977843465/build/Release/general-mean-quality-test.exe`.
  Exactly three copied assets are replaced: mean,difference,residual. The new
  general adaptation shader is checked independently in `limit_acceptance`;
  it is not substituted into that earlier full-quality caller.

Two initial builds of the new limit caller failed before numerical work:
missing `bin/shared` include directory, then use of the abstract instance type.
Both were corrected in the new test target/caller. The notes are preserved in
`build_limit_first_failure.txt`. There were no numerical mismatch fixes or
tolerance changes in this checkpoint.

## Reproduction and artifacts

Source-only shader compilation uses fresh output directories:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_dimensions/compile_mean_general.ps1 -OutputDirectory <mean> -VulkanBin C:/VulkanSDK/1.4.363.0/Bin
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_dimensions/compile_mean_general_difference_pack.ps1 -OutputDirectory <pack> -VulkanBin C:/VulkanSDK/1.4.363.0/Bin
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_dimensions/compile_residual_general_extended.ps1 -OutputDirectory <fft> -VulkanBin C:/VulkanSDK/1.4.363.0/Bin
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_dimensions/compile_base_general.ps1 -OutputDirectory <adaptation> -VulkanBin C:/VulkanSDK/1.4.363.0/Bin
```

Build `general-base-pipeline-test`, `general-spyr-test` and
`base144-pipeline-test` from `tests/hdrvdp3_residual144`. Build
`IQM-general-base-limit-test` from `tests/hdrvdp3_evaluation` using the existing
bounded evaluation toolchain. All test targets use strict floating arithmetic.
The common diagnostic header runs explicit shaders synchronously; no host
mean, DC subtraction or FFT replaces those GPU computations.

Generate new native captures with `generate_general_pathway_reference.m`
using `--visual --shape 80x80`; omitting shape retains the old two rectangles.
Use the five patterns above. `export_native_base_general.m` takes the new
capture root, existing private square-quality root and a fresh fixture output.
`export_square_spyr_reference.m` additionally accepts an optional shape string;
its default remains96x96. Old references are not rewritten.

Saved outputs can be audited without rerunning numerical work:

```powershell
node tests/hdrvdp3_evaluation/audit_base_general.cjs validation/base_general_639286707604688716
```

Relevant subdirectories: `acceptance`, `spyr_acceptance`, `limit_acceptance`,
`quality_acceptance`, `v2_pipeline_regression`, `production_regression`.
`fixtures` precedes the added limit export; `fixtures_limits` reproduces its
mean/pack/FFT goldens exactly and adds log/adaptation/Float64 limits. Source,
binary and artifact hashes preserve lineage; don't attribute old broad suites
to a later relinked executable.

## Next step

Implement and validate the new80x80 optical/local-adaptation path with exact
160x160 padding. Record the native FFT plan, input/layout/precision and
normalization, then compare forward/filter/inverse stages and retinal/P/log
outputs before joining the accepted SPYR chain. Only promote a public profile
after full image-to-score and CLI/runtime reproduction/regressions pass.
RGB, arbitrary dimensions, viewing conditions, other devices and fully
GPU-resident execution remain open.
