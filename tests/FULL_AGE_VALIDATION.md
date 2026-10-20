# Full pipeline age validation

Updated 2026-10-19. The local opt-in metric path now passes calibrated
age-dependent L/M/R coefficients consistently through CPU JND, GPU
adaptation preparation and GPU JND. Observer age also reaches pupil,
CSF band LUTs and base-band CSF/noise. The native validators use the
exported age rather than a fixed age24 value.

The harness accepts `IQM_OBSERVER_AGE=24/60/60.001`, default24.
`IQM_SPECTRAL_SAMPLES` points to the pinned native-resampled D65 input
fixture; nondefault ages require it. The runner exposes `-ObserverAge`
and `-SpectralSamples`, records the sample hash and separates age-specific
export prefixes. `spectral_lmr.txt` records the double coefficients.
Raw spectral-file loading/resampling is still not ported.

## Measured results

Ten fresh full metric pairs at30ppd, square32/64, D65 positive synthetic
luminance pass strict native stages and direct official Q/Q_JOD equality.
CPU and GPU thirteen-stage JND traces match. Raw SPYR bands and centered
residuals are checked separately. Independent GPU comparisons total
**1,254,496 numeric float values and52,352 binary64 comparison-mask words,
zero differences**. Numeric float equality is not a signed-zero bit claim.

| Case | Size | Age | Q | Q_JOD |
| --- | ---: | ---: | ---: | ---: |
| asymmetric localized |32|24|8.3328084945678711|8.9990530014038086|
| asymmetric localized |64|24|9.1295385360717773|9.5646781921386719|
| asymmetric localized |32|60|8.4544048309326172|9.091609001159668|
| asymmetric localized |64|60|9.2667655944824219|9.6505765914916992|
| asymmetric localized |32|60.001|8.4543228149414062|9.0915470123291016|
| asymmetric localized |64|60.001|9.2663593292236328|9.6503286361694336|
| stronger localized |32|60|2.6284239292144775|3.2776408195495605|
| stronger localized |32|60.001|2.6283564567565918|3.2775626182556152|
| HDR edges phase shift |32|60.001|2.774634838104248|3.4479899406433105|
| HDR edges phase shift |64|60.001|3.4441509246826172|4.2154660224914551|

Age24 controls preserve216 common text-export SHA256 hashes against the
previous accepted binary (102 at32,114 at64). New-age tests include the
optical-density formula branch at60 and a synthetic HDR range0.001..10000.
Different ages produce different scores; all listed scores equal the
corresponding official run, not the age24 result.

## Shader and equivalence

Map shader ABI13 replaces hardcoded adaptation L/M constants with supplied
single coefficients. JND receives calibrated L/M/R coefficients through its
existing packed data. The CPU single path accepts the same matrix, with
legacy age24 defaults. Older ABI12 shaders are rejected by the new host;
saved ABI12 binaries and exports remain intact.

ABI13 SHA256:
`871E3916717BFFCC49B4E59AD9E52EB311A148E277D99F32DCEAA8FE1C2FC0FE`.
SPIR-V validation passes; only Shader capability is present (no Float64 or
Int64). The FFT/base shader backends and their layouts are unchanged.
Text maps are row-major; FFT buffers and reduction order are column-major.
Visual area is width*height/ppd². MTF uses symmetric post padding and local
adaptation uses replicate post padding. Residual DC removal is not used to
hide differences in raw bands.

## Evidence and remaining scope

`validation/full_age_completed_path.txt` selects
`validation/full_age_639280300414897968/COMPLETED.json`. The root preserves
pre-change source/binary snapshots, fresh shader sources, build logs,
verified executables, source hashes, suite manifests and exact scores.
`validation/finalize_full_age_checkpoint.ps1` requires all ten suites,
native scores, raw/centered bands, GPU counts and pinned binary/input hashes
before issuing PASS. Mixed application/GPU integration remains local and
uncommitted; this checkpoint does not imply a completed public API.

The subsequent [raw spectral/viewing continuation](RAW_SPECTRAL_VALIDATION.md)
now validates four combined60ppd/new-age HDR cases and two raw-input pipeline
regressions. It also resolves a genuine one-ULP power rounding failure and
retains the failed artifacts. This original ten-case checkpoint remains pinned
to its original shader and native-resampled inputs. Next broaden
full-pipeline observer ages. Real HDR/color
inputs, arbitrary full-pipeline dimensions, other devices and public API
integration remain unfinished. Earlier fixed-age records remain historical;
see [spectral calibration](SPECTRAL_AGE_VALIDATION.md) for its separate scope.
