# Rectangular maps and first full-quality diagnostic

Checkpoint: `validation/rectangular_maps_639282910800030657`.
This extends component coverage with explicit `MapGPUProfile::RectangularMaps`
(ABI16). Public `evaluate_quality` still rejects rectangular inputs. The full
rectangular caller exists only with `IQM_HDRVDP3_DIMENSION_DIAGNOSTIC` enabled.

The actual MapGPU component runs in `candidate_gpu/` pass:

- 128 masking cases: 696,320 Float32 raw/pooled values numerically exact.
- 160 Float32 resize cases: 487,424 values numerically exact.
- 64 binary comparison-mask resize cases: 174,080 binary64 values bitwise exact.

Inputs cover 64x128/128x64 and their 32x64,16x32,8x16/transposed bands.
Mask pooling uses zero outside the image; resize follows official Octave
`imresize` bicubic/antialias/symmetric behavior. Inputs and exports are row-major;
these components perform no DC removal. Float32 signed-zero bits are not asserted.
Native fixtures are generated independently by
`generate_rectangular_maps_reference.m`, using official HDR-VDP-3 v3.0.7.

ABI16 selects pooling traversal by the longer axis and FMA behavior by the
larger dimension. ABI13/14/15 retain their guards and binaries; regenerated
ABI14/15 hashes match previous checkpoints. Accepted ABI16 SPIR-V SHA256:
`338EB96787DFAFFC10CC2034DCE767090C4379570C86A8EA3FA6C24E9A1D4614`.

Two full diagnostic pairs use the prior asymmetric luminance inputs, with a
localized test change (`reference*1.4+0.25`), 30ppd, age24, D65, Float32 and
native single-thread-count 3/estimate FFT planning. Each reference/test uses
the same dimensions and viewing parameters. MTF/LA use explicit post-padding,
column-major FFT spectra/filter transpose, inverse scaling once; residual FFTs
use explicit 8x16/16x8, no padding, native mean/DC removal and first-max selection.

| Height x width | Q | Q_JOD |
| --- | --- | --- |
| 64 x 128 | 4.2055172920227051 | 5.0616965293884277 |
| 128 x 64 | 3.6234698295593262 | 4.4173946380615234 |

Both scores match independent official results bitwise. Full intermediate
validation is recorded separately in `exact_stages_*/results.json`; score
equality alone does not establish intermediate parity. Both canonical runs pass
all six exact checks: preprocessing, SPYR, CSF, masking, differences and official
quality. The initial `full_*`
launch used the wrong PFM filename and failed before evaluation; only
`full_fixed_*` are numerical runs.

The original tall-image validator failed: preceding JND diagnostic FFTs left
cached single FFTW plans that changed the native reference (94/215 retinal
values differed). A minimal fresh-process check matches both MTF outputs
exactly. The explicit `--fresh-fft-plans` option resets single wisdom and sets
3 threads immediately before the full native chain. This reproduces independent
fixture conditions; it does not relax any equality assertion. The original
failed logs remain preserved. The quality validator needs the same reset before
its official call because earlier diagnostic plans can also change final scores.

Current-source public 32/64/128 regressions retain 96/107/107 numerical export
hashes and score bits. Nineteen public invalid-input cases and twenty MapGPU
profile/ABI/dimension/domain rejection cases pass. Executables and source hashes
are frozen in the checkpoint. Previous component acceptance is separate from
these two unique full diagnostic pairs.

Reproduce components with `tests/hdrvdp3_fft256/validate_rectangular_maps.ps1`
and a fresh output directory. Generate ABI16 with `generate_map16_shader.ps1`.
Generate full references with `generate_rectangular_quality_reference.m`;
build `IQM-quality-rectangular-diagnostic` in the diagnostic CMake configuration.
Run all six exact checks using
`tests/hdrvdp3_evaluation/validate_rectangular_quality.ps1`, which explicitly pins
the native FFT planning conditions and requires a fresh validation log directory.
No claim covers arbitrary dimensions, RGB inputs, other devices or public
rectangular quality support.
