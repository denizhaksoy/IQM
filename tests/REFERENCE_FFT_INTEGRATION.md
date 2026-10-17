# Experimental reference FFT preprocessing integration

> Publication scope: this document describes locally validated work. The implementation, new test tools and full artifacts are not included in this documentation checkpoint. See SESSION_HANDOFF.md at the repository root for current status; earlier measurements below retain their historical scope.


Subsequent version-pinned math experiments now pass all current strict stages
and full preprocessing checks. See [math validation](ADAPTATION_MATH_VALIDATION.md).
The baseline below describes reference FFT with native GPU log/exp and products;
its adaptation differences remain valid for that configuration.

The main HDR-VDP test harness can explicitly load the verified FFT parity
shader instead of VkFFT. This is a fixed64x64 input /128x128 FFT numerical
experiment; the default backend is still VkFFT and preprocessing defaults to
none. The runtime shader is derived from the separately licensed FFTW test
fixtures; it is not silently embedded in the production library.

```powershell
# First build IQM and compile the verified test shader; preserve old binaries.
$env:IQM_PREPROCESSING='fft'
$env:IQM_FFT_BACKEND='reference'
$env:IQM_FFT_AXIS_ORDER='column'
$env:IQM_FFT_TWIDDLES='native'
$env:IQM_FFT_REFERENCE_SHADER=(Resolve-Path 'validation/fft_forward2d_initial.spv').Path
$env:IQM_SPYR_PATTERN='asymmetric' # also diagonal, impulse
& './build-vs2026/bin/Release/IQM.exe' --method HDRVDP3 --enable-validation
# Set an ABSOLUTE export path printed by this run: nested Octave run() changes cwd.
$exportPath=(Resolve-Path '<export directory>').Path
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet --no-gui tests/diagnose_fft_stages.m $exportPath --strict-transforms
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet --no-gui tests/validate_fft_preprocessing.m $exportPath --strict
```

Reference backend requires column axis order and rejects the separate VkFFT
LUT experiment. Prepare, forward, multiplication and normalized inverse traces
all have column-major storage; finish crops accordingly. Exports include
`fft_backend.txt=reference` and `fft_inverse_storage.txt=column`. Diagnostic
scripts honor actual inverse storage instead of assuming a row-major inverse.
Forward/inverse use separate scratch SSBO and immutable descriptor sets with
compute barriers. Reference mode does not initialize or execute a VkFFT plan.

## Measured result

All3 patterns /6 images:

- Every MTF stage (filter, padding, forward FFT, multiply, inverse, crop/clamp)
  matches official single Octave exactly.
- Full MTF retinal output and L+M adaptation input:24576 values each, zero error.
- Full pupil coefficients and JND: all6 coefficients /24576 JND pixels equal.
- All raw SPYR bands:57216 values equal, including raw and centered residuals.
- Integrated same-input forward/inverse transforms pass strict assertions for
  MTF and adaptation on all6 images. Separate native JND intermediate checks pass.
- No Vulkan validation errors found. The existing default VkFFT adaptation
  regression still passes its tolerance (max0.00015259, RMSE0.00002840); this is
  not an exact parity claim for that backend.

**Full preprocessing still fails exact parity** because adaptation maps differ:

| Pattern | Ref adaptation max error | Test adaptation max error | Total differing adaptation pixels |
|---|---:|---:|---:|
| Asymmetric | 6.67572021484375e-6 | 5.7220458984375e-6 | 4426 |
| Diagonal | 5.7220458984375e-6 | 1.049041748046875e-5 | 4885 |
| Impulse | 8.58306884765625e-6 | 6.67572021484375e-6 | 4437 |

First divergence is GPU log during adaptation prepare (max2.384185791015625e-7).
Same-input filtered spectrum also differs on subnormal products (max about
1.36e-38); same-input exp at finish differs by up to1.9073486328125e-6.
Forward and inverse FFT themselves now have zero differences. Unchanged JND
and SPYR outputs for these mild stimuli do not establish equivalence of the
adaptation maps or more demanding content. Do not waive the strict failure.

Evidence under validation/:

- `fft_reference_backend_build.log` (completed build).
- `fft_reference_backend_{asymmetric,diagonal,impulse}_run.log`.
- `fft_reference_backend_{asymmetric,diagonal,impulse}_stages.log` and
  `*_strict_transforms.log` (strict transform PASS).
- `fft_reference_backend_asymmetric_full_absolute.log`,
  `fft_reference_backend_{diagonal,impulse}_full.log` (strict full-chain exit1
  due to the documented adaptation differences).
- `fft_reference_backend_*_native_jnd.log`.
- `fft_reference_backend_legacy_build.log`, `fft_reference_backend_legacy_regression.log`.
- Exports: `spyr_spatial_17922599150543331` (asymmetric),
  `spyr_spatial_17922599673027534` (diagonal),
  `spyr_spatial_17922599723530707` (impulse).

The initial asymmetric full-chain invocation used a relative directory and
failed to find pupil.txt after nested Octave run() changed cwd; it is not a
numerical result. The absolute-path rerun above completed all comparisons.

Next match native single log/exp rounding and subnormal spectrum products,
then repeat these strict comparisons before progressing to CSF/resizing/masking.
