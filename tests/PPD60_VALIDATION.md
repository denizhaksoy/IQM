# Viewing parameter validation: 30 and 60 ppd

Updated 2026-10-19. This records local, opt-in integration against official
HDR-VDP-3 v3.0.7 in Octave 11.3.0 on the current Intel Vulkan device. It is
not a claim of arbitrary viewing parameters or a completed public metric API.

The full metric harness accepts `IQM_PIX_PER_DEGREE=30/60`, defaulting to 30.
The runner exposes `-PixelsPerDegree 30/60` and exports
`pixels_per_degree.txt`. The shared band-count helper returns ppd as its
second output; old exports without this metadata default to 30. Unsupported
values are rejected. The 60 ppd path requires the FFT preprocessing and
Vulkan map backends.

The same ppd drives MTF, local adaptation, CSF band frequencies, physical
residual frequencies and the pupil's visual area (`width*height/ppd^2`).
Both sizes retain their size-limited pyramid depths: four bands for 32x32,
five for 64x64. The isolated older FFT/SPYR probes remain fixed at 30 ppd.

## Exact comparison scope

Evidence is selected by `validation/ppd60_completed_path.txt`; its directory
contains `COMPLETED.json`, per-suite paths, source snapshots/hashes, binary
hash manifests, saved executables and all logs. The finalizer requires every
suite to finish before issuing PASS.

Ten fresh pairs cover asymmetric localized distortion at both sizes and
both ppd values, plus six 32x32 stress cases at 60 ppd: horizontal/vertical
phase shifts, stronger localized distortion and three comparison-threshold
gains. Eight pairs use 60 ppd; two are 30 ppd regression controls. Across
these pairs, 912,478 numeric float values and 38,016 binary64 mask values
match, with zero differences. Strict full preprocessing, raw SPYR bands,
centered residuals and official Q/Q_JOD must also match independently.

| Input | Size | ppd | Q | Q_JOD |
| --- | --- | --- | --- | --- |
| asymmetric localized | 32 | 30 | 8.3328084945678711 | 8.9990530014038086 |
| asymmetric localized | 32 | 60 | 8.4778785705566406 | 9.1092472076416016 |
| asymmetric localized | 64 | 30 | 9.1295385360717773 | 9.5646781921386719 |
| asymmetric localized | 64 | 60 | 9.3098649978637695 | 9.6766700744628906 |
| horizontal phase shift | 32 | 60 | 6.3274946212768555 | 7.2468290328979492 |
| vertical phase shift | 32 | 60 | 6.3275036811828613 | 7.2468376159667969 |
| stronger localized | 32 | 60 | 2.5939817428588867 | 3.2373738288879395 |

## Additional HDR stress checkpoint

`validation/ppd60_hdr_completed_path.txt` selects a separate four-pair
checkpoint: HDR edge phase shifts and stronger localized HDR ramps at both
sizes, spanning 0.001 to 10,000 cd/m². It adds 570,202 float and 23,808
binary64 mask comparisons with zero differences; strict raw/centered SPYR,
all native stages and official scores also match. Four independent exported
input geometry checks pass. Combined viewing checkpoints cover 14 fresh
pairs, 1,482,680 float values and 61,824 double mask words without differences.

| Input | Size | ppd | Q | Q_JOD |
| --- | --- | --- | --- | --- |
| HDR edge phase shift | 32 | 60 | 2.9746236801147461 | 3.6794271469116211 |
| HDR ramp stronger localized | 32 | 60 | 9.1807851791381836 | 9.5972375869750977 |
| HDR edge phase shift | 64 | 60 | 3.7124395370483398 | 4.5169939994812012 |
| HDR ramp stronger localized | 64 | 60 | 9.9616508483886719 | 9.9920291900634766 |

## Ordinary HDR transforms completed

`validation/ppd60_hdr_standard_completed_path.txt` selects eight additional
pairs: HDR ramp/edges with localized distortion, ramp with global brightening,
and edges with stronger global brightening, each at 32x32 and 64x64. These
use the same verified application/checker hashes as the viewing checkpoints
above. All native stages, raw/centered SPYR bands and official scores match.
Independent GPU comparisons add **1,140,404 floats and 47,616 binary64 mask
values, zero differences**. All eight exported input geometry checks pass.

The three viewing checkpoints together contain 22 fresh pairs: 20 at 60 ppd
and two 30 ppd controls. They total 2,623,084 float comparisons and 109,440
double mask comparisons. Repeated runs are not unique input-case coverage.
The full score table and hashes are in the individual checkpoint records.

## Equivalence and limits

Official comparisons consume the actual exported single-precision luminance,
with age 24 and the existing D65 single-channel calibration. MTF uses symmetric
post padding; local adaptation uses replicate post padding. Text maps are
row-major; FFT buffers are column-major interleaved complex. Inverse FFT
normalization is applied once. Residual DC subtraction is tested separately
from the raw bands, so centering cannot hide a raw-band offset.

Numeric float equality does not establish identical signed-zero bits. Binary64
mask comparisons do check bits. Shader/binary identities are pinned per suite;
GPU filenames use the exported four/five band count. Preserve old directories
and use a fresh export for each rerun.

The prior [32x32 HDR/stress checkpoint](FFT32_VALIDATION.md) remains separate:
its canonical 18 labels span two accepted binary groups at 30 ppd. Do not add
repeated regression runs to unique case coverage. Observer-age changes,
real HDR/color input, arbitrary dimensions, other devices and public
API behavior remain to be validated. Observer age requires consistent pupil,
AESL/CSF and spectral/lens handling; changing only the pupil is insufficient.
