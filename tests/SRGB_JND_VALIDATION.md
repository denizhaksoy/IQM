# Double sRGB pupil and JND validation

Validated 2026-10-28. Completed root is recorded in
`validation/srgb_jnd_completed_path.txt`.

Added `rgb_to_jnd_response_double` with row-major Double LMSR/adaptation
inputs, native column-major geometric-mean accumulation, unified pupil
reduction, post-pupil response, native-grid linear JND interpolation and
combined achromatic response P. The existing Float32 and legacy Double
sampling APIs are unchanged. The new function uses the actual Double LUT
grid and slope/offset interpolation rather than an approximate uniform index.

The reference capture now exports actual post-pupil LMSR, geometric mean,
pupil reduction, corrected LUTs, all four JND output planes and combined P.
The fourth JND plane is zero; the third contains the rod response. Native
stock/instrumented Q/Q_JOD bits remain equal under all three spectra for
each of two distinct original encoded Single pairs: ramps/threshold patches
and deterministic stress codes/primaries/adjacent threshold values.

Both pairs use official HDR-VDP-3 v3.0.7, Octave11.3,96x96,30ppd,age24,
defaultsp0 and Double FFTW3threads3/estimate/cleared Double wisdom. Layout
conversion from native column-major captures to the row-major API is explicit.
Padding, DC and inverse normalization are unchanged from optical validation.

Each isolated stage test compared534,540 Double words with zero bit errors:
corrected LUTs, geometric means, pupil factors, post-pupil LMSR, JND planes
and P. Across both pairs that is1,069,080 words; repeated identical LUTs are
included in these counts. Six invalid-input cases per run also passed
(dimensions, area, nonpositive LMSR/adaptation and duplicate LUT nodes).

The independent local-FFTW CPU diagnostic was extended through JND/P.
It starts from original encoded Single images and independently computes
EOTF, color matrix, optical processing, spectral response, local adaptation,
pupil and JND using source-generated LUTs. No native intermediate, matrix,
filter or LUT supplies the calculation; reference captures supply comparisons.
For each pair it compared4,478,976 prior optical/adaptation Double words plus
497,664 post-pupil/JND/P words, all bitwise exact. Total9,953,280 words across
two pairs and three spectra. The isolated-stage counts overlap this scope
and must not be added as independent content coverage.

The diagnostic explicitly loads local Octave FFTW with unaligned planning;
it is not a production/GPU FFT implementation. Normal IQM Release and strict
MSVC tests built successfully. Full sRGB Q/Q_JOD prediction has not yet been
implemented independently: Double pyramid, masking and pooling remain, as
does production Double FFT/runtime integration. CLI sRGB remains unsupported.

```powershell
$r = Get-Content validation/srgb_jnd_completed_path.txt
& "$r/srgb-jnd-double-test-used.exe" "$r/native_ramps" "validation/srgb_jnd_recheck_$([DateTime]::UtcNow.Ticks)"
```

Next implement the Double steerable pyramid with native Double filter
coefficients, reflect1 boundary handling, band normalization and base-band
DC removal. Existing Float32 filter literals cannot establish Double parity.
