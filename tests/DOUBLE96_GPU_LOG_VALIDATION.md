# Production Double96 JND logarithms (runtime v10)

Validated 2026-10-31. Root:
`validation/double96_log_639290557075459411`.

The optional `-GPULogarithm` packaging flag enables runtime v10 and implies all
earlier GPU flags. Its 22 manifest payloads comprise 16 shader modules and six
model files; original MIT notices accompany the package. Default remains v1.
Vendored pinned AMD win-libm source enables offline shader generation.

## Behavior and retained CPU boundary

The backend's optional sixteenth pipeline evaluates interleaved natural/decimal
logarithms. `logarithms()` accepts 1 through 36,864 positive normal finite
Doubles and rejects other domains before GPU dispatch. Its output fits the
existing 73,728-Double buffer. It serializes work using the backend mutex/fence.

With v10, JND adaptation log values and clamped L/M/rod log10 queries come from
this GPU pipeline. The CPU retains column-order summation, geometric-mean
`exp`, pupil model and interpolation slope preparation. Earlier runtimes retain
their CPU logarithm path. The quality GPU stage remains enabled for v10.

Other visual preprocessing logarithms, CSF nonlinear model preparation and
quality powers remain CPU operations. This is a further hybrid stage, not a
fully GPU metric. No CPU fallback silently substitutes for a rejected argument
to the explicit logarithm API.

## Acceptance measurements

- 1,493,028 native JND comparisons: 1,492,992 GPU array values and 36 CPU
  geometric-mean/pupil scalars, all exact.
- 414,730 JND control comparisons: 414,720 GPU array values and 10 CPU scalars.
- 1,344,072 direct production log-API outputs exact against the preserved Octave
  query oracle, including table/exponent/near-one boundaries.
- Total **3,251,830 comparisons, zero bit differences**: 3,251,784 GPU-returned
  values and 46 CPU scalars. Direct log coverage repeats the preceding isolated
  corpus and must not be counted as new images or added to earlier totals.
- Fifteen API/stage rejection checks passed.
- Nine public CLI configurations retained 72 exact quality Doubles and 497,664
  decoded Float32 input words; thirteen CLI rejection checks passed.
- Runtime v1 through v9 retained 27 byte-identical numeric/input export files.
- Fourteen malformed v10 runtime cases were rejected before GPU creation.
- Inspected Vulkan validation logs were clean.

The production logarithm SPIR-V is byte-identical to the accepted isolated
FMA-order shader. Native intermediate arrays are independently checked after
column-to-row conversion. The final audit also rechecks scores, log API bytes,
runtime regressions, feature metadata and vendored source identity.

The CLI corpus is the existing two distorted pairs and identity across three
displays, fixed 96x96/30 ppd/age 24/default sp0. Execution used the NVIDIA RTX
A500 Laptop GPU. No generic dimensions, viewing conditions, denormal handling,
all-input rounding or other-device parity is claimed.

## Reproduction and next step

Build runtime with `build_srgb96_double_runtime.ps1 -GPULogarithm` and a fresh
output directory. Build target `double96-log-test`; arguments are
`<v10-runtime> <native-cpu-root> <isolated-log-root> <fresh-output>`.

```powershell
node tests/hdrvdp3_evaluation/audit_double96_log.cjs <root> <isolated-log-root>
```

Public CLI, legacy and runtime-rejection commands are saved under the root.
An initial CLI setup failure caused by PowerShell JSON metadata is retained in
`public_v10_setup_failed`; the corrected fresh `public_v10` is the accepted run.

Next: apply the same verified logarithm stage to the remaining visual/quality
log query preparation, with full intermediate and score checks; then investigate
the source-defined GPU `exp` and `pow` stages. Preserve older runtime paths and
keep nonlinear CPU boundaries explicit until individually validated.
