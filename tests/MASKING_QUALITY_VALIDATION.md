# Masking and quality validation

2026-10-20 clean-source continuation:the bounded runtime is committed in
7c7f473 and builds in a detached clean worktree with clean VkFFT. Two fresh
direct32/64 cases preserve score bits and203 numerical export hashes;14 invalid
cases pass. External reference shader binaries were copied and hashed,not
regenerated. See [clean-source checkpoint](CLEAN_CHECKOUT_VALIDATION.md).

2026-10-20 library API continuation: `evaluate_quality` now runs the validated
quality path from the HDRVDP3 library with explicit luminance inputs,
viewing/age and assets, without environment parsing or synthetic generation.
Ten fresh full regressions preserve official Q/Q_JOD and strict stages:
1,425,505 float values and59,520 binary64 masks, zero differences;1,218
text exports remain identical. Two independent direct callers ignore a
poisoned environment, return bitwise matching scores and preserve203 numeric
exports. Fourteen invalid API configurations/inputs are rejected. Scope is
still square32/64 luminance;general dimensions, RGB-native and clean-checkout
runtime packaging remain open. See [API checkpoint](EVALUATION_API_VALIDATION.md).
This supersedes older public-API-pending statements only within that scope.

Current2026-10-19 extension:six32x32 full-metric pairs now match official
Q/Q_JOD and strict intermediates;the four-band GPU checker compares342276
float values and14208 double masks with zero differences. Six64x64 regressions
also pass. See [full32 checkpoint](FFT32_VALIDATION.md). This supersedes the
earlier pending32x32 implementation notes below within this specific scope.

2026-10-19 follow-up:after generalized raw SPYR arithmetic,eight fresh64x64
pairs (original6,HDRstress2) still match official Q/Q_JOD and all strict
intermediates. The new32x32 mask-resize checker independently rechecks those
eight exports with zero differences. Full32x32 quality is not yet implemented.
See [dimension checkpoint](SPYR_DIMENSION_VALIDATION.md);the eighteen-pair
checkpoint below retains its previous binaries and historical scope.

> Current scope: scalar CSF/resize code and isolated checks are committed locally in `51525e0` and `7ce32a0`;GPU integration and full evidence remain local/uncommitted. No push was performed in this session. Earlier measurements below retain their historical scope.

2026-10-18 fresh regression:all18 synthetic pairs pass strict preprocessing,
CSF,masking,raw/final D,native noise and direct official Q/Q_JOD checks using
the same application/checker binaries after the CSF LUT correction. GPU
intermediate comparisons include4104990 float values and171648 double mask
values with zero differences. This includes low-quality phase/strong-localized
cases and below/nominal/above comparison-mask threshold cases. Evidence:
`validation/lut_grid_regression_639279511058157010/COMPLETED.json`.
The scope is64x64/30ppd/age24/D65 positive synthetic luminance;broader metric
coverage remains unfinished. Historical checkpoints below are preserved.


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

## Local ABI12 stress-validation continuation (2026-10-18)

The latest local summary is tests/HDRVDP3_VALIDATION_SUMMARY.json; details and
an eight-case score/mask/frequency table are in tests/MAP_GPU_VALIDATION.md.
Eight fresh periodic/phase-shift,stronger localized and threshold-gain pairs
match official Q/Q_JOD exactly,including Q=3.4347231388092041 and
Q=3.6115429401397705. Native active comparison-mask counts around gain1.001f
are0,4056,4096 out of4096. Single raw/final D,masks,double base noise and
bitwise double mask resizing pass,not just the rounded final score.

The original ten cases and additional eight retain separate evidence/hashes.
Combined:4104990 single GPU values and171648 binary64 mask values,zero
differences. This publication notice remains applicable:these latest changes
have not been committed/pushed and do not expose a completed public metric API.
Full scope is still64x64/30ppd/age24/D65 positive luminance,synthetic inputs.

## Later size, HDR and viewing-parameter checkpoints (2026-10-19)

The scope statements above describe historical checkpoints. Subsequent
[32x32 validation](FFT32_VALIDATION.md) adds stress, HDR and corrected
three-impulse coverage, with 17 fresh pairs and zero numeric differences.
The canonical 32x32 matrix has 18 labels across two accepted binary groups.
[Viewing-parameter validation](PPD60_VALIDATION.md) records 22 fresh
30/60 ppd pairs, including both HDR transform groups, with exact intermediate
and official Q/Q_JOD comparisons. [Observer-age component checks](OBSERVER_AGE_VALIDATION.md)
add isolated pupil and double CSF/AESL validation. The later
[full age checkpoint](FULL_AGE_VALIDATION.md) verifies ten full pipeline
pairs atage24/60/60.001 and30ppd, using pinned native-resampled D65 inputs.
The [raw spectral/viewing continuation](RAW_SPECTRAL_VALIDATION.md) adds four
60ppd/new-age HDR pairs,a corrected power rounding corner and two raw-loader
application regressions. These scopes retain separate hashes and counts.
The portable JSON retains these checkpoints separately; mixed application
integration remains local, and the public metric API remains incomplete.

## File-based luminance continuation (2026-10-20)

[File-input validation](LUMINANCE_INPUT_VALIDATION.md) now verifies six
Nancy Church derived-luminance crop pairs andtwo synthetic regressions at
32/64,30ppd,age24,D65. All native stages andscores match;the combined eight
runs compare1,140,404 GPU floats and47,616 binary64 mask words exactly.
Independent source-file bit checks cover the input route. Crop/color/unit
assumptions are explicit;full-frame andRGB-native metric parity remain open.
