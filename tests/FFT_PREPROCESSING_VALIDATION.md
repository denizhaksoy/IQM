# FFT preprocessing integration (2026-10-16)

> Publication scope: this document describes locally validated work. The implementation, new test tools and full artifacts are not included in this documentation checkpoint. See SESSION_HANDOFF.md at the repository root for current status; earlier measurements below retain their historical scope.


## Follow-up: axis-order and FFT input-type investigation

Correction from the subsequent native-plan probe: `fftw('threads')` reports
3 for the double planner; actual single planning uses 22 threads on this host.
See `tests/FFTW_PLAN_VALIDATION.md` for the exact independently verified native
plan and why this matters. Row-first GPU forward results are much closer to
Octave row-first sequential transforms than to its native real-input fft2.
For the asymmetric reference, forward spectrum max error was
0.007682190371777677 vs real fft2, and 4.464988674438334e-5 vs row-first.
This diagnoses traversal order, not indexing or scaling.

Forward FFT stage comparisons now use `fft2(real(gpu0))`: constructing a
complex array with zero imaginary data selects a numerically different FFTW
path. The earlier stage log's `forward_same_input` values used that complex
path. Full-chain comparisons already used real inputs and are unaffected.

The optional `IQM_FFT_AXIS_ORDER=column` transposes prepare storage and then
transposes the filtered spectrum back before the inverse. This second
transpose matters: VkFFT's inverse traversal reverses its forward order.
Transfer-only element copies transpose exactly, without float arithmetic.
An initial experiment that transposed only prepare/finish worsened full-chain
results and is retained in `validation/fft_column_*` for provenance.

With both directions aligned, asymmetric-image full MTF maxima became
2.86102294921875e-6 (reference) and 1.9073486328125e-6 (test), versus
3.814697265625e-6 for both in the row-first baseline. JND maxima remained
0.000244140625; mismatches fell from 1171/882 to 444/433. This is an
experimental partial improvement, not exact parity. Evidence:
`validation/fft_aligned_stages.log` and `fft_aligned_asymmetric_comparison.log`.

Prepare/finish push constants now have four uint32 fields (16 bytes).
Snapshots 0/1/2 are transposed in column mode; snapshot 3 is row-major after
inverse alignment. `fft_inverse_storage.txt` marks the new layout; diagnostics
still support older column-mode snapshots without this marker.

`IQM_FFT_TWIDDLES=lut` additionally enables VkFFT precomputed twiddles for
controlled numerical comparison (default `native`, LUT disabled). A plan's
twiddle mode cannot change after initialization. Neither experimental setting
changes the default behavior. See the session handoff for the latest results.

The first verified LUT attempt exposed an existing null fence handle passed
to VkFFT. Its LUT uploader requires a real fence for submit/wait/reset. The
implementation now owns that fence for the plan lifetime; post-fix runs are
under `validation/fft_lut_fixed_*`. Earlier failed/invalid attempts are labelled
in `validation/fft_lut_asymmetric_INVALID.md` and must not be used as LUT results.
After the fix, asymmetric MTF forward-spectrum maxima vs real fft2 were
2.750817318316642e-5 (reference) and 3.075507258719844e-5 (test). Final MTF
maxima remained 2.86102294921875e-6/1.9073486328125e-6, JND mismatches were
434/411 (max error 0.000244140625), and no Vulkan errors were logged.
Remaining differences require tracing actual real-input FFTW codelets and
butterfly arithmetic; LUT/axis selection alone does not establish parity.

Column-aligned/LUT regression also completed on diagonal and impulse patterns.
All six shared-input native JND and shared-JND SPYR checks passed; no Vulkan
errors were logged in post-fix runs. Full-chain MTF maxima over all six images
are 2.86102294921875e-6, full JND maxima 0.000244140625, and all six full
reference pupil coefficients match. The complete preprocessing still has
nonzero mismatches. Legacy default FFT integration remains within its prior
tolerance after the fence fix (same max=0.00016022, RMSE=0.00002903).

## Earlier integration baseline

The experimental `IQM_PREPROCESSING=fft` harness now runs GPU optical MTF,
native single LMSR L+M adaptation input, GPU FFT local adaptation, native CPU
single pupil/JND, and GPU SPYR. Default preprocessing remains `none`, preserving
the earlier isolated tests. Scope: 64x64, 30 pixels/degree, age 24, D65 luminance.

The existing `LocalAdaptFFT` implementation also supports optical MTF mode;
both modes share its float32 VkFFT transform/multiply path. Gaussian filter
construction now follows the reference's double arithmetic before conversion
to single. Shader prepare/finish modes are explicit:

- MTF uses symmetric post-padding and crop/clamp, matching official
  `surround='none'`. The old standalone MTF test used replicate padding and
  therefore did not establish default-surround parity.
- Local adaptation uses log/clamp, replicate post-padding, crop/exp. This FFT
  path rejects sigma below 4.3, where the official code uses spatial filtering.

## Measured full-chain differences

These are maxima over reference/test images for each pattern, compared against
fresh official functions on the original luminance (not shared GPU retinal/JND).

| Pattern | MTF output max | Adaptation max | JND max | Raw residual max |
|---|---:|---:|---:|---:|
| Diagonal | 5.7220458984375e-6 | 8.58306884765625e-6 | 0.0003662109375 | 0.001953125 |
| Asymmetric | 3.814697265625e-6 | 1.049041748046875e-5 | 0.000244140625 | 0.001953125 |
| Impulse | 3.814697265625e-6 | 1.049041748046875e-5 | 0.000244140625 | 0.0029296875 |

All six shared-input pupil/JND trace comparisons and all six shared-JND SPYR
comparisons still matched exactly. Full reference pupil coefficients matched
in five cases; diagonal reference differed by 1.1920928955078125e-7.
No Vulkan VUID/validation error messages were found in the three GPU run logs.
This is **not an exact full-chain parity pass**.

The existing ramp-based `IQM-la-fft-integration-test` also completed within its
existing tolerances: MaxAbsErr 0.00016022, RMSE 0.00002903. That tolerance pass
does not establish exact parity. The MTF-off asymmetric JND/SPYR regression
also passed after these changes.

## First divergent operations

`diagnose_fft_stages.m` compares four complex snapshots per pass on identical
inputs, plus filter and image output. Asymmetric results:

- MTF filter, prepared/padded input, multiplication on the same FFT input,
  and crop/clamp on the same inverse buffer match exactly.
- The first MTF mismatch is forward FFT. Forward-spectrum maximum complex
  error is 0.007682190371777677 (reference image). Inverse FFT on the identical
  filtered buffer also differs (maximum complex error 1.910704710363661e-6).
- Adaptation filter coefficients match exactly. GPU log differs from Octave
  by up to 2.384185791015625e-7 before FFT. Forward/inverse FFT also differ.
  Multiplication has subnormal differences up to about 1.35e-38. GPU exp on
  the identical inverse real buffer differs by up to 1.9073486328125e-6.

These measurements localize arithmetic differences; they do not prove that
changing VkFFT configuration or increasing precision will reproduce Octave's
single FFT rounding. Matching operation ordering/rounding is the next task.
Do not change shaders based only on final-band mismatches.

## Reproduce

From the workspace root in PowerShell:

```powershell
cmake --build build-vs2026 --config Release --target IQM IQM-jnd-single-test
# Targeted shader build; do not run compile_shaders.sh (it deletes shaders_out).
glslc shaders/hdrvdp3/hdrvdp3_fft_prepare.glsl -o shaders/hdrvdp3/hdrvdp3_fft_prepare.spv --target-env=vulkan1.2
glslc shaders/hdrvdp3/hdrvdp3_fft_finish.glsl -o shaders/hdrvdp3/hdrvdp3_fft_finish.spv --target-env=vulkan1.2
$env:IQM_PREPROCESSING='fft'
$env:IQM_SPYR_PATTERN='asymmetric' # also diagonal, impulse
$env:IQM_FFT_AXIS_ORDER='column' # experimental; default row
$env:IQM_FFT_TWIDDLES='lut' # experimental; default native
& './build-vs2026/bin/Release/IQM.exe' --method HDRVDP3 --enable-validation
# Supply the export directory printed by this invocation:
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet --no-gui tests/validate_fft_preprocessing.m '<directory>' --strict
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet --no-gui tests/diagnose_fft_stages.m '<directory>'
& './build-vs2026/bin/Release/IQM-jnd-single-test.exe' '<directory>'
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet --no-gui tests/validate_spyr_spatial.m '<directory>'
Remove-Item Env:IQM_PREPROCESSING
Remove-Item Env:IQM_SPYR_PATTERN
Remove-Item Env:IQM_FFT_AXIS_ORDER
Remove-Item Env:IQM_FFT_TWIDDLES
```

The full-chain script's `--strict` mode currently fails as expected; without
it, the script reports differences for diagnosis and only asserts shared-input
checks. Local evidence: `validation/fft_*_comparison.log`,
`validation/fft_stage_comparison.log`, `validation/fft_strict_check.log`,
and corresponding GPU, JND, SPYR logs. Prior shader binaries were backed up in
`validation/fft_shaders_before_*`. Original Octave source and old C:/temp
references were preserved.
# Subsequent experimental reference backend

The verified FFT arithmetic has now been connected to an explicit
`IQM_FFT_BACKEND=reference` runtime-shader path. All6 images' MTF, L+M input,
pupil/JND and raw SPYR outputs match; adaptation maps still differ, so strict
full preprocessing still fails. See [integration evidence](REFERENCE_FFT_INTEGRATION.md).
Earlier VkFFT-only measurements below retain their historical scope.
