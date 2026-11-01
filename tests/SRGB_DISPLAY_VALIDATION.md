# sRGB display CPU preprocessing

Validated on 2026-10-28 against official HDR-VDP-3 v3.0.7 in Octave 11.3.
The completed artifact root is recorded in
`validation/srgb_double_completed_path.txt` after final checks.

`prepare_srgb_display` accepts three planar Single encoded channels in
[0,1], independently calibrates the BT709-to-native matrix from raw CIE1931
and display emission samples, and returns Double display RGB, raw native
RGB and native RGB clamped to 1e-6. It is a CPU preprocessing API, not a
complete sRGB quality prediction or GPU implementation. CLI `srgb-display`
remains unsupported until the downstream Double pathway is implemented.

The native display model is mixed precision: Single division/addition/power
expressions are assigned into a Double zeros array, followed by Double
`99*RGB+1`. Using Double for all EOTF expressions caused 163,680 mismatches
out of 165,888 values in the first characterization fixture. The accepted
implementation preserves Single intermediates and does not fuse the final
scale/add. Double matrix multiplication follows the measured native forward
FMA accumulation order. Independent matrix coefficients matched all 54
captured words (three matrices, checked twice across two sides).

Two distinct 96x96 encoded pairs were tested under `led-lcd-srgb`,
`led-lcd-wcg` and `oled`, with 30 ppd, age 24, default sp0 and native
Double FFTW three threads/estimate/cleared Double wisdom. The first pair
contains ramps, trigonometric variation, threshold patches and local
distortions. The stress pair contains deterministic varied codes, 0/1,
pure red/green/blue, black and adjacent Single codes around 0.04045.
Original Single inputs and native Double EOTF/matrix/raw/clamped outputs
are captured in fresh directories; old reference exports are unchanged.
Stock and instrumented native Q/Q_JOD bits agree for both fixtures and all
three spectra. No captured intermediate or matrix is supplied to the API.

Each fixture passed 497,664 Double-word binary comparisons across EOTF,
raw conversion and clamped conversion: **995,328 words total**, including
signed-zero bits. Both runs also rejected 18 invalid inputs (six cases
repeated across three spectra); these are 36 checks, not 36 distinct cases.
This is exact agreement within the tested preprocessing scope, not a
tolerance pass or arbitrary-platform exactness guarantee.

The isolated MSVC `/fp:strict` test and normal IQM Release build succeeded.
No shader, runtime manifest, existing quality path or CLI routing changed.
The sandbox initially could not discover MSVC; the same CMake test build
then succeeded through the approved escalation route. The initial failed
configuration directory and logs are preserved.

```powershell
$r = Get-Content validation/srgb_double_completed_path.txt
& "$r/srgb-display-test-used.exe" "$r/native_stress" C:/HDR-VDP-3_Ref/data "validation/srgb_recheck_$([DateTime]::UtcNow.Ticks)"
```

Next: preserve Double through optical processing, local adaptation, JND,
steerable pyramid, masking and pooling before exposing `srgb-display`
through the production quality API/CLI. Existing Float32 runtime assets
cannot establish exactness for that Double path. Full prediction and
cross-device coverage remain unverified.
