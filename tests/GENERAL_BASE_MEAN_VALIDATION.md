# Variable-size base-band mean and DC removal

Checkpoint: 2026-10-27. Evidence root:
`validation/mean_general_639286695977843465`.

`mean_general.comp` removes the fixed120/144 element assumption from this
isolated GPU stage. It computes the Float32 mean and centered band using
two uint32 words for the exact signed sum and integer long division with
round-to-nearest-even. No shader Float64 support is needed.

The caller must check: count1..4096; finite normal Float32 or signed zero;
unbiased exponents in[-90,100], with a span at most16 within each group.
The absolute sum uses at most52 magnitude bits, so the same input sum is
exactly representable in the native binary64 accumulator. Very small
cancellation results are normalized before division. Subnormals, nonfinite
inputs and wider exponent spans are outside this contract. The shader checks
count but relies on the caller for the input-domain check.

## Measured scope

Tested counts:1,2,3,7,64,100,120,144,255,256,1024,4096. This samples the
declared count domain; it is not an exhaustive GPU test of all4096 lengths.

- `acceptance_bounds`:12,288 bounded synthetic groups and144 independent
  Octave `mean(v(:))`/Float32 subtraction groups; **6,303,024 Float32 words
  bitwise exact**, including signed zeros. Native probes include the maximum
  accumulation range and near-total cancellation at the lower exponent bound.
- `official_spyr144`:8,192 synthetic groups plus10 original official
  HDR-VDP-3 v3.0.7 SPYR base-band fixtures;1,187,840+1,450 words exact.
- `quality_acceptance`:alternate diagnostic integration on the same six96x96
  pairs as the public acceptance. **414 checks / 1,711,662 Float32/Float64
  values exact**, including Q/Q_JOD, all69 checked stages per pair, selected
  residual, base noise and band errors. No new independent image cohort.
- No reported Vulkan validation errors in these runs.

The first `acceptance`/`native` cohort has eight native probes per count
(6,278,688 words); it remains preserved. `native_bounds`/`acceptance_bounds`
adds four extreme-range probes per count. Do not attribute the expanded
cohort to the earlier test executable: each run records its own binary hash.

The alternate full-chain caller uses
`IQM_HDRVDP3_GENERAL_MEAN_DIAGNOSTIC` only. It passes the actual base count
in the second mean push word. The production v1/v2 mean ABI and payload hashes
remain unchanged. All other14-asset-package shaders are copied from the
previous accepted private square96 assets; only `mean.spv` is replaced.

## Reproduction

Use fresh output directories and the documented Windows toolchain:

```powershell
cmake -S tests/hdrvdp3_residual144 -B <fresh-build> -G "Visual Studio 18 2026" -A x64
cmake --build <fresh-build> --config Release --target mean-general-test general-mean-quality-test mean144-test -j4
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_dimensions/compile_mean_general.ps1 -OutputDirectory <fresh-shader> -VulkanBin C:/VulkanSDK/1.4.363.0/Bin
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet tests/export_native_mean_general.m <fresh-native>
node tests/hdrvdp3_evaluation/validate_mean_general.cjs <fresh-build>/Release/mean-general-test.exe <fresh-shader>/mean_general.spv <fresh-native> <fresh-results>
```

The full-chain test uses `validate_square_quality.ps1` with the alternate
executable,14 copied assets with only the mean shader replaced, and the
existing native reference root `validation/square_quality_639286340181932107`.
The official SPYR replay uses `mean144-test` with the new shader and the
retained `mean144_639286139772659192/mean_fixtures_complete/manifest.txt`.

## Exact next step

Follow-up: [general base pipeline](GENERAL_BASE_PIPELINE_VALIDATION.md) now
validates difference packing, small FFT/selection, adaptation and diagnostic
SPYR including a new80x80 source geometry. The steps below describe the
historical next step of this mean-only checkpoint; see the follow-up report
for the current next step and its exact acceptance boundaries.

Generalize the difference-pack and residual-FFT/magnitude/selection stages
under a separately versioned diagnostic ABI, first against native base bands
from a new geometry. Check FFT layout, signed-zero narrowing, DC treatment,
actual selected bin/frequency and adaptation before promoting any dimension.
The mean component alone does not enable additional public image dimensions.
RGB, viewing-condition generalization and fully GPU-resident processing remain
open; existing CPU model/LUT/NCSF and host transfers are unaffected.
