# Adaptation math experiments (local, version-pinned)

> Publication scope: this document describes locally validated work. The implementation, new test tools and full artifacts are not included in this documentation checkpoint. See SESSION_HANDOFF.md at the repository root for current status; earlier measurements below retain their historical scope.


Follow-up: the Windows binary dependency has been removed in a source-backed
generator using original MIT-licensed AMD win-libm inputs. See
[source-backed math](math_reference/README.md). The binary-probe measurements
below retain their historical scope.

This work addresses the remaining differences after reference FFT integration:
adaptation log, exp, and subnormal spectral products. Defaults still use native
GPU math/VkFFT. Experimental shaders are loaded only through explicit runtime
paths. No portable log/exp replacement is claimed by these measurements.

## Native reference identification

Installed Octave liboctave-13.dll imports logf/expf through
api-ms-win-crt-math-l1-1-0.dll. Local import/export and arithmetic inspection
identified the AVX/FMA UCRT routines, not the generic MinGW logf wrapper.
The inspected ucrtbase.dll is10.0.26100.9444, SHA256:
`5C52E3A303BAAAC0E0AF8BD9B96134993DA34BC9D834A31EF37E1D2CDC7FE192`.

The local diagnostic generator `validation/generate_ucrt_log_probe.ps1` rejects
other DLL hashes. It creates a version-specific log/exp shader from the observed
arithmetic and native tables. Binary-derived tables and probe artifacts remain
local under validation/; they are not a shipped portable math implementation.
The exp probe emulates its double intermediates using compensated float pairs
with split products, without requiring shaderFloat64.

Increasing mathematical accuracy alone did not establish native parity:

| Experiment | Log mismatches | Exp mismatches | Scope |
|---|---:|---:|---|
| Initial compensated FMA-residual series | 6881 | 30049 | 65536 pairs |
| Explicit split-product compensated series | 21 | 3 | Same pairs |
| Native log graph + series exp | 0 | 3 | Same pairs |
| Native log + native exp graph probe | 0 | 0 | Same pairs |

The series result sometimes agrees with double-to-float rounding while native
logf differs by one last bit near a midpoint. Native arithmetic order therefore
matters. These observations do not establish correctly-rounded or universal
equivalence for every possible input.

## Isolated GPU results

- Log/exp:65536 pairs, including24576 actual adaptation-input/inverse-output
  pixel pairs from the three spatial patterns. Synthetic log inputs span
  [1e-6,1e10]; exp inputs span[-80,80], plus near-one/zero cases. Native probe
  matches all131072 function outputs, max error0, RMSE0, mismatches0.
- Multiplication:262144 finite pairs, including196608 actual Gaussian-spectrum
  products. Integer significands with umulExtended and explicit ties-to-even
  rounding preserve gradual underflow, including subnormal operands/results,
  signed zeros, underflow ties, signs and normal results. All products match
  native single Octave exactly. NaN/Inf behavior is outside this experiment.

Original test sources: float_math.comp (series experiment, not a native parity
pass), float_multiply.comp, and their generate_float_*_reference.m scripts.
The shared GPU test harness has --float-math and --float-multiply modes.
The math fixture pairs store log input in x, exp input in y; these are separate
scalar tests, not complex FFT data. Overall exit status requires zero mismatches.

## Integrated explicit probe

Set IQM_FFT_MATH_SHADER_DIR to a directory containing prepare.spv, finish.spv
and multiply.spv. This requires IQM_FFT_BACKEND=reference, column axis order,
native twiddle option, plus the verified reference FFT shader path. Empty math
directory preserves existing math. Configuration changes after initialization
are rejected. Export fft_math_shader_dir.txt records the actual selected path.

The local complete probe directory is
`validation/ucrt_complete_math_639278575887831116/`. The first math-only probe
directory ucrt_math_shaders_639278574065405048 had prepare/finish only; after
adding the multiply override it is not a complete directory for the current
loader. Do not reuse it with the updated loader.

All3 patterns /6 current64x64 images passed both strict modes: every filter,
prepare, forward, multiplication, inverse and finish comparison is equal;
the full original-input retinal/L+M/adaptation/JND maps have zero differences.
Pupil coefficients and all5 SPYR bands, including raw/centered residuals, also
match the official native Octave outputs. No Vulkan validation errors found.
This is scoped to the recorded30-ppd, age24, D65 luminance fixtures and the
pinned UCRT/device environment; it is not a full metric or arbitrary-input pass.

```powershell
$env:IQM_PREPROCESSING='fft'
$env:IQM_FFT_BACKEND='reference'
$env:IQM_FFT_AXIS_ORDER='column'
$env:IQM_FFT_TWIDDLES='native'
$env:IQM_FFT_REFERENCE_SHADER=(Resolve-Path 'validation/fft_forward2d_initial.spv').Path
$env:IQM_FFT_MATH_SHADER_DIR=(Resolve-Path 'validation/ucrt_complete_math_639278575887831116').Path
$env:IQM_SPYR_PATTERN='asymmetric' # diagonal, impulse
& './build-vs2026/bin/Release/IQM.exe' --method HDRVDP3 --enable-validation
$exportPath=(Resolve-Path '<new export directory>').Path
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet --no-gui tests/diagnose_fft_stages.m $exportPath --strict-all-stages
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet --no-gui tests/validate_fft_preprocessing.m $exportPath --strict
```

## Artifacts

- math reference: validation/float_math_reference_639278571099031674/.
- multiply reference: validation/float_multiply_reference_639278575349483287/.
- isolated logs: float_math_initial_comparison.log, float_math_dekker_comparison.log,
  ucrt_log_probe_comparison.log, ucrt_log_exp_probe_comparison.log,
  float_multiply_comparison.log under validation/.
- builds: float_math_build.log, float_math_diagnostic_build.log,
  float_multiply_build.log, ucrt_math_integration_build.log,
  ucrt_complete_math_build.log under validation/.
- First math-only asymmetric run: spyr_spatial_17922606838939870, with
  ucrt_math_asymmetric_stages.log (log/exp equal, subnormal products differ),
  ucrt_math_asymmetric_full.log (full exported preprocessing maps already equal).
- Complete probe exports: spyr_spatial_17922608381270175 (asymmetric),
  spyr_spatial_17922608432949170 (diagonal), spyr_spatial_17922608486247470 (impulse).
  Logs: ucrt_complete_<pattern>_{run,stages,full,native_jnd}.log.
- Default backend regression: ucrt_complete_default_regression.log, tolerance
  pass with max0.00015259, RMSE0.00002840. This is not default-backend exact parity.

The original official reference and existing staged/untracked work are preserved.
Keep this probe local until a maintainable, portable source-backed math implementation
and broader numerical coverage are established. Full metric CSF/resizing,
masking/pooling and score validation remain separate work.
