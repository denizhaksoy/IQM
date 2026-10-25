# Masking and quality validation

2026-10-24 experimental image-to-score assembly and fresh single-command replay
pass for both80x96/96x80 localized-distortion fixtures. P/log are GPU-generated
from GPU MTF/LA; pupil is GPU-computed; no image-dependent native intermediate
is used as a runtime value in the replay. Fixed model data/axis plans and CPU
double NCSF remain. Final Q/Q_JOD match bitwise. This is file/host-composed
experimental quality validation,not new public geometry support. See
[image-to-quality checkpoint](ACTUAL_GPU_COMPOSITION_VALIDATION.md).

2026-10-24 base-noise runtime dependency removed: GPU base120 and residual
selection drive existing double NCSF evaluation.126 checks/393384 values match,
including noise and Q/Q_JOD; all240 native double frequency-grid values match
bitwise. Native P/log-adaptation still remain. Fresh upstream visual-pathway
reference stages are exported with native score/P/log identity checks. See
[base-noise checkpoint](GPU_BASE_NOISE_VALIDATION.md).

2026-10-24 GPU-derived masking inputs: native reference/test bands,self masks
and CSF runtime maps replaced by accepted GPU outputs. Full composition passes
122 checks/393374 values with zero numerical and observed bit differences;
both Q/Q_JOD pairs match. Native P/log-adaptation and base noise remain, and
components still use host transfers. No full metric/public-shape claim. See
[GPU band/masking checkpoint](GPU_BAND_MASKING_VALIDATION.md).

2026-10-24 general comparison masks:90 cases pass691200 binary word checks and
321840 double resize values bitwise, with no host intermediate computation.
Feeding actual GPU double-mask dumps into the masking/quality checker passes
82 checks/250334 values and preserves Q/Q_JOD. Composition still uses native
self masks,CSF and base noise; public general quality remains disabled. See
[comparison checkpoint](GENERAL_COMPARISON_VALIDATION.md).

2026-10-24 native-input masking/quality components:72 checks/214574 values
match numerically, observed bit differences zero. Neighbor,N_mask,D,
psychometric,D_p,band errors and Q/Q_JOD pass for actual80x96/96x80 pairs.
This relies on native intermediate masks/CSF/base noise; complete image-to-score
GPU integration and general comparison-mask resize remain pending. See
[masking checkpoint](PATHWAY_MASKING_VALIDATION.md).

2026-10-24 actual pathway resize/CSF:10band maps,35760 resize outputs match;
resize+CSF test compares71520 values with zero numerical errors. Isolated
general-shape mutual masking/pooling also passes71520 outputs on the same
10bands. Production masking still rejects these shapes; experimental per-tap
integer-FMA pooling does not widen public support. Full quality remains pending.
See [resize/CSF/pool checkpoint](PATHWAY_RESIZE_CSF_POOL_VALIDATION.md).

2026-10-24 full SPYR and joined residual processing: all sp0Filters bands plus
the DC/difference/FFT/magnitude/index chain pass together from actual P maps,
113526 comparisons across two image pairs, no host intermediate computation
or upload. Observed bit differences are zero; convolution/FFT acceptance is
numerical, mean/DC/packing/magnitude/index acceptance bitwise. Separate modes
also regress successfully. This ends at residual selection, not full quality.
See [joined SPYR checkpoint](SPYR_LOWPASS_VALIDATION.md).

2026-10-24 GPU SPYR lowpass: actual P maps from both80x96/96x80 pathway pairs
pass all four lowpass stages,40800 Float32 values numerically exact, with
observed bit differences zero. Native reconstruction also reproduces raw and
DC-centered base bands bitwise. Highpass/orientation bands and connection to
the residual GPU chain remain pending; no general public quality acceptance.
See [lowpass checkpoint](SPYR_LOWPASS_VALIDATION.md).

2026-10-24 combined residual chain: GPU mean/DC -> centered test-reference
subtraction/packing -> FFT/magnitude/index passes both actual pathway pairs,
with no host intermediate computation/upload.484 mean/DC,480 packed difference
and242 magnitude/index words match bitwise;480 FFT components match
numerically. Previous four-band and12-band checker modes also pass. Upstream
SPYR/visual-pathway inputs are still native; full GPU quality remains pending.
See [combined GPU checkpoint](RESIDUAL120_VALIDATION.md).

2026-10-24 actual pathway residuals: four reconstructed raw base bands reproduce
the official centered bands bitwise. GPU mean/DC -> packing -> FFT/magnitude/
index passes all four. Two independently uploaded actual band differences
also match480 FFT components numerically and240 magnitudes/two indices
bitwise, including the traced native selection. Original12-case pipeline and
76-matrix residual regressions pass. The complete GPU subtraction chain and
full quality integration remain pending. See
[real pathway residual checkpoint](RESIDUAL120_VALIDATION.md).

2026-10-24 component checkpoints: both 160x192 and 192x160 forward/inverse
FFT orientations now have exact numerical comparisons. Bounded mean120/DC
passes bitwise, and the actual GPU mean/DC -> packing -> residual FFT ->
magnitude/index chain passes on 12 native-derived cases. The extended residual
suite covers 76 matrices: 18240 raw FFT floats numerically exact, 9120
magnitudes and 76 first-max indices bitwise exact. A fresh run with the latest
checker also preserves the raw mode pass (490 signed-zero differences only).
Base120 mean/power passes 16408 bitwise output comparisons across 8204 maps.
These are component acceptances; general public quality is still pending.
Two additional actual full-pathway resized logmaps (80x96/96x80,30ppd,age24)
now pass base120 mean/power bitwise. Native trace instrumentation preserves
both stock Q/Q_JOD exactly; these are not GPU score comparisons.
See [transposed FFTs](TRANSPOSED_MIXED_FFT_VALIDATION.md),
[mean/DC](MEAN120_VALIDATION.md), [residual chain](RESIDUAL120_VALIDATION.md)
and [base adaptation](BASE120_VALIDATION.md). Earlier entries below retain
their historical validation scope.

2026-10-23 complete160x192 inverse:12 streams match737280 raw native FFTW
values and737280 normalized official ifft2 values exactly on Intel Arc Pro
Graphics. First-axis and forward regressions also pass. Signed-zero bits are
outside this FFT check; transposed/general public quality remain pending. See
[complete inverse checkpoint](MIXED_INVERSE_VALIDATION.md).

2026-10-23 inverse160 first axis:12 native MTF/LA streams match737280
unnormalized Float32 values exactly on Intel Arc Pro Graphics with explicit
DenormPreserve32/RTE32. Complete inverse/general public quality remain pending;
signed-zero bits are outside this FFT check. See
[inverse first-axis checkpoint](INVERSE160_AXIS_VALIDATION.md).

2026-10-23 complete160x192 forward FFT:12 native MTF/LA streams match737280
Float32 values exactly on Intel Arc Pro Graphics, with373248 first-axis
regression values also exact. Inverse/general public quality remain pending.
Signed-zero bits are outside this FFT check. See
[mixed forward checkpoint](MIXED_FORWARD_VALIDATION.md).

2026-10-23 real160 first axis: isolated GPU forward half-spectrum matches
12 native MTF/LA streams on Intel Arc Pro Graphics,373,248 Float32 values
with zero numerical differences. Signed-zero bits are not asserted here.
Second axis/inverse and general public quality remain pending. See
[160-point axis checkpoint](MIXED160_AXIS_VALIDATION.md).

2026-10-23 mixed FFT continuation:24 native MTF/LA streams pin equivalent
160x192/192x160 execution graphs. New fixed inverse-normalization GPU code
matches1,054,696 synthetic and1,474,560 native Float32 words bitwise. GPU FFT
graphs/general public quality remain pending. See
[mixed FFT checkpoint](MIXED_FFT_PLAN_VALIDATION.md).

2026-10-23 general-dimension frontier: eight new native full-quality references
pin exact FFT/residual geometry. An isolated arbitrary-size GPU padding/layout
stage passes96 cases and14,843,520 Float32-word bit comparisons onNVIDIA RTX A500.
Public dimensions remain unchanged. See
[general-dimension checkpoint](GENERAL_DIMENSION_FRONTIER.md).

2026-10-23 explicit CLI: PFM pair,runtime,ppd,age,spectral data and fresh export
can be supplied directly. Five supported shapes preserve524 export hashes with
poisoned legacy settings;22 parser rejections and a60ppd/age60.001 full native
check pass. See [CLI checkpoint](CLI_QUALITY_VALIDATION.md).

2026-10-23 publish preparation: a selected-source snapshot with clean VkFFT
builds the API and optional application target,regenerates19 pinned shaders,
preserves620 smoke export hashes and passes35 rejection checks. Root multi-metric
build experiments remain outside this scope. See
[publish preparation](PUBLISH_READINESS.md).

2026-10-23 application PFM entry: matching32/64/128square and64x128/128x64
luminance files now select their dimensions directly,with an explicit
IQM_QUALITY_RUNTIME package. Eleven application pairs preserve1,166 numerical
exports;synthetic32/64 strict regressions andseven invalid-input checks pass.
See [application checkpoint](APPLICATION_LUMINANCE_VALIDATION.md).

2026-10-23 public rectangular API: evaluate_quality now accepts64x128/128x64
at30ppd/age24 using explicit FFT64256/map16/base85 assets. A fresh normal build
preserves official score bits and856 export hashes across8 pairs. Square
32/64/128 regressions and35 rejection checks pass. See
[public rectangular checkpoint](RECTANGULAR_PUBLIC_API_VALIDATION.md).
This supersedes earlier public-rectangle-pending statements for these two sizes.

2026-10-22 rectangular stress extension: six more full pairs (HDR ramp,
impulse and identity in both orientations) pass 36 exact stage checks and
official score bits. Identity gives zero error and Q=Q_JOD=10. All six runtime
shaders regenerate with pinned identical hashes. Total full rectangular scope
is eight pairs; public rectangle support remains pending. See
[stress checkpoint](RECTANGULAR_STRESS_VALIDATION.md).

2026-10-22 rectangular maps: ABI16 passes 696,320 masking and 487,424 resize
Float32 values exactly, plus 174,080 binary64 mask values bitwise. First full
64x128/128x64 diagnostic scores match official results bitwise; public API
rectangles remain rejected. See [rectangular maps](RECTANGULAR_MAPS_VALIDATION.md)
for scope and separate intermediate validation evidence.

2026-10-21 inverse256 component:18 isolated GPU executions preserve2,359,296
float components exactly;the same first-axis exports independently match the
native FFTW oracle. Explicit DenormPreserve32/RTE32 is required;default-control
LA first-axis differences are retained as failed evidence. Full GPU128 quality
andforward FFT256 remain open. See [inverse checkpoint](FFT256_INVERSE_VALIDATION.md).

2026-10-21 source-to-shader checkpoint:all7 reference shaders now regenerate
from committed FFTW/AMD/helper sources with identical binary hashes. Clean
build/direct32/64 API checks preserve203 numerical exports andscore bits;
14 rejection cases pass. See [shader reproduction](REFERENCE_ASSET_VALIDATION.md).
Native128-input/256-FFT diagnostics also identify8 exact unaligned CPU plans;
this does not enable GPU128 quality. See [frontier](FFT256_FRONTIER.md).

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

## 128-square integration continuation (2026-10-22)

The later [rectangular MTF/LA integration](RECTANGULAR_FILTER_VALIDATION.md)
and [128-sample residual integration](RECTANGULAR_RESIDUAL_VALIDATION.md)
add LocalAdaptFFT ABI64256 and explicit MapGPU ABI15/85 residual components.
Rectangular masking/resize and continuous score validation remain open;
the public quality API still requires its validated square dimensions.

The later [rectangular component checkpoint](RECTANGULAR_COMPONENT_VALIDATION.md)
validates sixteen SPYR dimension cases and isolated128x256/256x128 forward,
inverse and8x16/16x8 residual FFTs. Continuous rectangular quality/API support
still requires MTF/LA andMapGPU integration.

The later [128 integration record](HDR128_INTEGRATION_VALIDATION.md) documents
the explicit FFT256/map14/base82 runtime contracts and three historical full
native stage/score checks. This extends the local quality API to128-square
positive luminance at30ppd/age24. Earlier32/64 scope statements remain
historical; other128 viewing conditions and general dimensions remain open.

## File-based luminance continuation (2026-10-20)

[File-input validation](LUMINANCE_INPUT_VALIDATION.md) now verifies six
Nancy Church derived-luminance crop pairs andtwo synthetic regressions at
32/64,30ppd,age24,D65. All native stages andscores match;the combined eight
runs compare1,140,404 GPU floats and47,616 binary64 mask words exactly.
Independent source-file bit checks cover the input route. Crop/color/unit
assumptions are explicit;full-frame andRGB-native metric parity remain open.

### Reusable pathway runtime components (2026-10-25)

The accepted mixed MTF/LA,SPYR+mean/DC/residual,and mutual pooling implementations
now have in-memory library entry points. Their standalone native comparisons
retain exact arithmetic/layout contracts. See MIXED_FILTER_RUNTIME_VALIDATION.md,
SPYR_RUNTIME_VALIDATION.md andPOOL_RUNTIME_VALIDATION.md for counts and scopes.
Public quality geometry guards remain unchanged. Existing production MapGPU
continues resize/CSF; comparison/masking composition and full API integration
remain to be completed. Native image-dependent values are test goldens only.
