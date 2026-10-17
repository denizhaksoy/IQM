# Masking and quality validation

> Publication scope: this document describes locally validated work. The implementation, new test tools and full artifacts are not included in this documentation checkpoint. See SESSION_HANDOFF.md at the repository root for current status; earlier measurements below retain their historical scope.


The C++/Vulkan validation wrapper now exports the CSF, masking, raw masked
difference, psychometric difference and quality stages separately. The
original CPU reference baseline is described below. An opt-in Vulkan map
backend now also passes ten pairs with GPU CSF/resize/clamp/raw masking/pooling;
see [Vulkan map validation](MAP_GPU_VALIDATION.md) for current scope and artifacts.
Other stages still include CPU work. Existing opt-in FFT/math settings remain
required for the exact upstream chain; default backend settings are unchanged.

## Algorithms and precision

- Mutual activity is min(abs(reference),abs(test)) after the pyramid's
  power-of-two amplitude normalization and residual DC removal.
- The installed Octave single conv2 uses different accumulation paths on the
  current square pyramid bands: FMA on64/32-row bands, separate multiplication
  and addition on16/8-row bands. Tap traversal is columns ascending, rows
  descending. The wrapper explicitly follows those observed paths. This is
  version/device-specific validation behavior, not a general convolution rule.
  A broader rectangular probe already shows additional native paths; do not
  extend the current rule to other shapes without validation.
- Neighbor activity uses default bicubic/antialiased/symmetric resize,
  clamped to zero after each resize. Self and neighboring masking terms use
  single powers, multiplications and sums. sp0Filters has one orientation,
  so no cross-orientation activity exists in this scope.
- Masked excitation, denominator, division and psychometric powers use native
  single arithmetic, with the reference's scalar double base-band noise term
  converted at the mixed single addition.
  N_mask^2 uses multiplication, not powf(x,2): the latter failed one localized
  pixel due to different rounding and consequently changed both D outputs.
- Base-band sensitivity is no longer a hardcoded1.522517668677. It uses the
  dominant non-DC frequency and geometric adaptation mean. A direct double
  DFT selects the frequency; its spectrum is NOT claimed equal to native
  single fft2. Frequency selection must be tested on near ties/other sizes.
  The geometric luminance here is10^mean(log10(adaptation)); Octave mean uses
  double accumulation followed by a single mean. This differs from the
  sequential single sum used by the quality Minkowski operation.
- A single luminance query in official nCSF produces single interpolated
  parameters which are assigned to a double parameter matrix. The new
  compute_ncsf_single_luminance preserves that behavior. Its1152-case fixture
  has zero rounded float mismatches;399 double results differ by at most
  2.2204460492503131e-16. Base-band inverse sensitivity may also differ in its
  final double bits; downstream D maps are checked exactly as float32.
- Quality pools raw D BEFORE the psychometric sign-power used for visibility
  reconstruction. The old prototype incorrectly pooled the transformed
  bands. diff_mask comes from the original luminance pair's0.001 relative
  threshold; it is resized as double and multiplied into single D. Double
  resize passed45 cases/32643 pixels exactly, alongside the float version.
- Minkowski error is sum(abs(Dp)^0.8 / pixel_count)^(1/0.8), with division
  before the single column-major sum. Q_err, Q and Q_JOD preserve native
  mixed single precision. Summing first and dividing afterwards is different.

## Checks and artifacts

Each matrix export starts with rows/columns followed by row-major round-trip
float32 values. Validation scripts explicitly transpose on import. Filenames:
csf_log_adaptation.txt, csf_log_bandN.txt, csf_bandN.txt,
mask_raw_bandN.txt, mask_pool_bandN.txt, mask_neighbor_bandN.txt,
mask_denominator_bandN.txt, quality_difference_bandN.txt (raw D),
difference_bandN.txt (psychometric D). base_csf.txt holds dominant frequency,
mean luminance and inverse nCSF as17-digit scalars. quality.txt holds five
band errors followed by Q_err/Q/Q_JOD. test_transform.txt records the case.

Validation scripts do not modify the installed reference. They check exact
float matrix equality and direct scalar equality, not tolerances:

```powershell
octave-cli --quiet --no-gui tests/validate_csf_maps.m '<absolute export>'
octave-cli --quiet --no-gui tests/validate_masking_maps.m '<absolute export>'
octave-cli --quiet --no-gui tests/validate_difference_maps.m '<absolute export>'
octave-cli --quiet --no-gui tests/validate_quality_scores.m '<absolute export>' --official
```

The last script first checks staged pooling using exported raw D, then calls
the installed hdrvdp3('quality',test,reference,'luminance',30,options) directly
with use_gpu=false,age24. It checks Q/Q_JOD exactly and saves the returned
reference structure in the fresh export. It does NOT assert C_map/P_map,
reconstruction or detection-score parity.

`run_quality_validation.ps1` creates a fresh batch and tests three spatial
patterns with the original5% brighter pair, plus asymmetric identical,
localized8x8 (+5%) and globally stronger (+50%) pairs. It requires a completed
IQM build and explicit verified FFT/math shader paths:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tests/run_quality_validation.ps1 `
  -ReferenceShader '<verified FFT SPV>' -MathShaderDirectory '<verified math directory>'
```

The script writes validation/quality_suite_latest_path.txt, exports.txt and
individual logs; PASS.txt is written ONLY after all six cases and all checks
pass. A directory without PASS.txt is incomplete or failed. Keep historical
failed artifacts: initial quality pooling matched its staged test but failed
the direct official call, exposing the wrong choice of D for pooling.

Earlier masking-only three-pattern exact passes are recorded by
validation/masking_{asymmetric,diagonal,impulse}_final_path.txt and logs.
No general image-size/device/color/age parity, double CSF equality or complete
Vulkan metric implementation is claimed by these scoped experiments.

## HDR-range extension

`run_quality_validation.ps1 -HDR` runs four additional pairs: hdr_ramp with
localized +5% and global +5% changes, hdr_edges with localized +5% and global
+50% changes. Reference inputs span0.001–10000 cd/m²; the stronger test
reaches15000 cd/m². hdr_ramp varies log luminance across both coordinates;
hdr_edges uses eight luminance levels with offset horizontal/vertical edges.
Inputs are synthetic single-channel D65, not real HDR image files.

The default six-case suite is unchanged. HDR runs use a separate
validation/quality_hdr_suite_latest_path.txt pointer and fresh directories,
preserving previous passing exports and failed diagnostics. Both modes now
record the executable/FFT/math shader SHA256 hashes automatically and run
the13-intermediate native JND comparison as well as the existing checks.
Build IQM and IQM-jnd-single-test before running either mode. A rounded-node
JND lookup bug found by a separate18432-case boundary fixture was corrected;
see JND_PATH_VALIDATION.md. Current HDR and baseline regressions are rerun
after that numerical change; require PASS.txt for completed suite evidence.

After the lookup correction, all four HDR cases passed in
validation/quality_hdr_suite_639278605867642854 (PASS.txt). All13 native JND
trace values, strict upstream preprocessing/pyramid stages, CSF, masking,
raw/transformed D and direct official Q/Q_JOD comparisons pass. Scores below
are rounded only for display; assertions use exact equality.

| HDR case | Q | Q_JOD |
| --- | ---: | ---: |
| Ramp localized +5% | 9.9997100830 | 9.9999847412 |
| Edges localized +5% | 9.9998035431 | 9.9999904633 |
| Ramp global +5% | 9.9993886948 | 9.9999599457 |
| Edges global +50% | 9.9999618530 | 9.9999990463 |

The six original cases also pass after the JND correction in
validation/quality_suite_639278607136633457/PASS.txt. Together with the HDR
batch above, ten end-to-end pairs pass all checks, including all13 native
JND intermediates on both images of every pair. The separate18432-case knot
fixture passes with both reference tables and production-generated tables.
This broadens the luminance/input coverage; dimensions, viewing conditions
and CPU/GPU division remain unchanged. No final all-Vulkan metric claim is made.

## Completed six-case run (2026-10-17)

`validation/quality_suite_639278599874511853/PASS.txt` confirms all six cases
passed strict upstream preprocessing/JND/pyramid checks, CSF maps, masking
maps, both D variants, staged pooling and direct official Q/Q_JOD equality.
There is no tolerance in these pass conditions. The tested code/shader hashes
are recorded in source_hashes.json and binary_hashes.json; exports.txt lists
every fresh input/output directory.

| Case | Q (rounded for display) | Q_JOD (rounded for display) |
| --- | ---: | ---: |
| Asymmetric localized +5% | 9.1295385361 | 9.5646781921 |
| Asymmetric global +50% | 9.9965181351 | 9.9996309280 |
| Asymmetric global +5% | 9.9999570847 | 9.9999990463 |
| Diagonal global +5% | 9.9999551773 | 9.9999990463 |
| Impulse global +5% | 10 | 10 |
| Asymmetric identical | 10 | 10 |

The historical C:/temp/oct_jnd_spyr64_bandN.txt comparison is now opt-in via
IQM_LEGACY_PYRAMID_COMPARISON=1. Those constant-image files are not valid
references for fresh spatial runs. Their old log differences are not used
by any of the strict validation scripts above.
The logging guard was added after the six-case numerical suite, rebuilt and
checked again on the localized case: direct official Q/Q_JOD remain exact,
and the stale comparison is absent. Evidence:
validation/quality_legacy_guard_smoke_{run,quality}.log and
quality_legacy_guard_smoke_path.txt. No numerical helper changed after the
six-case pass.

Remaining work: generalize image dimensions/native conv2 paths; investigate
near-tie base-frequency selection and double CSF differences; validate real
HDR/color/age/viewing configurations; add reconstruction and visibility/detection outputs
where required. This milestone validates the quality-mode experimental chain
within the stated scope, not a completed public HDR-VDP-3 API for all inputs.

GPU continuation (2026-10-17): the opt-in MapGPU implementation now evaluates
CSF/resize, masking/pooling, neighbor/N_mask, both D transformations and quality
reduction/Q/Q_JOD in Vulkan, using native-style powers and explicit rounding.
The current ABI7 also moves pupil/JND and mean-adaptation log10 to GPU.
Double comparison-mask resize, base-band double noise, frequency/DC and
difference preparation remain CPU work. See [GPU validation](MAP_GPU_VALIDATION.md)
for the exact scope and per-stage/full-chain evidence; the earlier CPU and
rejected GPU-math measurements remain historical.
