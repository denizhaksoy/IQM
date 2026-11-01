# Original RGB to quality scores

2026-10-27 checkpoint: see `validation/rgb_quality_active_path.txt`.

The checker now composes original RGB input through GPU optical filtering,
independent C++ spectral calibration, GPU LMSR, adaptation, pupil, JND and P,
then the production SPYR/pooling/masking/quality chain and RGB comparison mask.
No native intermediate is injected into the calculation. Native files are
goldens only. CPU model/LUT preparation, double NCSF calculations and host
stage transfers remain; this is not fully GPU-resident.

`evaluate_pathway_rgb_response_quality` is a downstream API accepting computed
P/adaptation and original RGB96 row-major planar inputs. It requires the v2
geometry/map13 downstream assets plus an explicit RGB-capable comparison
shader. `finish_quality` is shared with the old luminance entry; that entry
retains its domain/profile guards. Its v4 competing_axes regression passes
69 checks / 507,149 Float32/Float64 values bitwise after this refactor.

## Validated scope

Official HDR-VDP-3 v3.0.7, Octave11.3, FFTW3 threads3/estimate/cleared single
wisdom; nonidentity synthetic96×96 Float32 `rgb-native`, age24,30ppd,default
quality/sp0. Three display spectra: led-lcd-srgb,led-lcd-wcg,oled. Fixtures
include unchanged zeros, a positive blue change inside a zero patch, pure
primary regions and local red/blue distortions. Symmetric MTF and replicate
local-adaptation padding, retained optical/adaptation DC, explicit layout
transposes, and accepted unpadded12×12 residual processing are preserved.

Stock/instrumented native scores have equal bits. Original-input GPU chain
`acceptance/` passes 884,748 Float32 comparisons including all six Q/Q_JOD
outputs. Native pairs and resulting scores:

| Display | Q | Q_JOD |
| --- | ---: | ---: |
| led-lcd-srgb | 8.650890350341797 | 9.236832618713379 |
| led-lcd-wcg | 8.56220817565918 | 9.171975135803223 |
| oled | 8.668749809265137 | 9.249752044677735 |

The additional `native_bands/` export captures official D_p per band, the
five local Minkowski errors and their accumulated Q error. It independently
checks that repeated accumulation reproduces the actual native Q_err after
each band. `acceptance_bands/` adds direct bit comparisons against these
outputs. `acceptance_bands/` passes **949,134 Float32 words bitwise**, including
64,368 masked band values and 18 band/accumulated error scalars beyond the
first run. Cohorts overlap. Both runs used Vulkan validation; no errors were
reported. Final executable/source snapshots correspond to this expanded run.

## Reproduction and limits

Build `rgb-optical-gpu-test` from `tests/hdrvdp3_residual144`. Generate fresh
native fixtures with `generate_rgb_projection_reference.m <fresh> --jnd`.
Pass arguments: native directory, v2 runtime/assets, official spectral data,
accepted projection SPIR-V, fresh output, adaptation SPIR-V, ABI17 JND map,
v2 runtime directory, RGB comparison SPIR-V. Active pointers from preceding
RGB checkpoints identify these assets; binaries/hashes are frozen here.

RGB96 results are three spectral configurations of one synthetic image pair,
not broad real-image or viewing-condition coverage. Full RGB preparation is
currently orchestrated in the checker; the downstream API is production code.
Next move preparation into a supported production RGB entry with explicit
assets, expose RGB PF inputs/encoding/runtime through API/CLI, and extend the
nonidentity cohort. No universal reference parity or whole-metric completion
claim. The two-week delivery priority remains the validated usable scope.
