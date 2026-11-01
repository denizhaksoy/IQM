# Independent XYZ/BT.709 calibration and production API

2026-10-28 checkpoint: `validation/xyz_matrix_active_path.txt`.

Production spectral code now loads CIE1931 XYZ and three display-primary
emissions using the native420-point grid/PCHIP order. `calibrate_display_xyz`
computes trapz(XYZ×E), applies683.002 after integration, then normalizes by
the native Y-row sum. `rgb709_transform` independently solves the3×3 system
against the official BT.709 matrix. Original files are explicit inputs; no
captured/native intermediate is read by these production functions.

## Measured calibration

Three displays (led-lcd-srgb,led-lcd-wcg,oled): **8,901 Float64 values bitwise
exact**:8,820 resampled wavelength/XYZ/emission values,54 raw/normalized matrix
entries and27 solved coefficients. The independently computed coefficient
files also match both actual full709 main-path matrix captures byte for byte.
Official3.0.7/Octave11.3 source instrumentation is transparent to the result.

Initial32 solve candidates did not match Double completely (best3/27 errors),
although every candidate rounded to matching Float32 coefficients. That was
not treated as exact parity. Expanded128 candidates identified flags77:
partial pivoting and reciprocal LU scaling, separate trailing updates,
forward dot accumulation with explicit FMA followed by one subtraction,
and reciprocal upper solve with separate descending rank-one updates.
This sequence matches all27 Double coefficients on the three tested displays.
It characterizes this native BLAS environment, not every LAPACK implementation
or arbitrary ill-conditioned primary set.

## Independent conversion and scores

The previous GPU conversion/clamp shader is unchanged. With the new independent
matrix, the original709-input test chain passes **442,374 Float32 words bitwise**:
raw/clamped conversion331,776, native P/adaptation110,592 and six Q/Q_JOD scores.
No captured transform matrix is injected in this run.

`evaluate_pathway_rgb709_quality` now performs that composition in production
code. Its own direct API test passes the same **442,374 Float32 words bitwise**.
It checks the XYZ and LMSR models describe identical wavelength grids/display
emissions, converts original row-major planar709 input on GPU, clamps at1e-6,
and calls the accepted native-RGB quality path with converted originals for
the comparison mask. CPU models/LUTs/NCSF and host copies remain.

Scope: one synthetic96×96 linear709 pair, three display spectra,30ppd,age24,
default quality/sp0; native3.0.7,Octave11.3,FFTW3 threads3/estimate/cleared single
wisdom, native symmetric MTF/replicate adaptation/DC rules. These overlap earlier
conversion cohorts; they are not three independent content pairs. GPU validation
is enabled and no errors were reported.

Q/Q_JOD: sRGB8.5618896484375/9.17173957824707,
WCG8.561656951904297/9.171567916870118,
OLED8.551936149597168/9.164388656616211.

## Regression and reproduction

Rebuilt existing RGB spectral test passes9 configurations/10,296 Double words
bitwise. Mono13-age checker has zero numerical mismatches in2,520 raw samples
and16,822 intermediate values; its numeric comparison is not a signed-zero
bit equality assertion. A rebuilt native RGB API regression is recorded in
`native_api_regression/` separately from709 acceptance.
It passes93 checks /949,134 Float32 words bitwise. The first full709 test
orchestration binary predates adding the production709 wrapper; its arithmetic
is unchanged. The final API709 binary and its acceptance use the final sources.

Native matrix fixtures: `generate_xyz_matrix_reference.m <fresh>`. CPU target
`xyz-matrix-test` in `tests/hdrvdp3_rgb` takes native directory, original raw
spectral data and fresh output. `color-solve-inspect` records candidate results.
Generate full709 fixtures with `generate_rgb709_reference.m <fresh>`.
Target `rgb709-quality-api-test` in `tests/hdrvdp3_residual144` takes native,
native RGB runtime, spectral data, accepted709 conversion SPIR-V, fresh output.

No709 CLI encoding/runtime is exposed yet. Next bind the converter in a distinct
validated runtime profile, add explicit709 CLI options, verify PF-to-score and
main application integration, then expand709 content. sRGB-display still needs
a separate Double precision investigation; it must not silently use this path.
