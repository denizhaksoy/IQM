# RGB spectral calibration validation

Validated on 2026-10-27 against official HDR-VDP-3 v3.0.7 using Octave 11.3.
Artifacts: `validation/rgb_spectral_639287141118400654`.

## Implemented scope

`load_rgb_spectral_samples` reads explicit cone, rod and three-primary display
files and reproduces the native 420-point 360–780 nm sampling and PCHIP
interpolation. `calibrate_rgb_spectral_age` applies age-dependent optical density,
integrates the three primaries against four sensors, and normalizes the entire
3×4 matrix by the sum of its L and M columns. Each primary is not normalized
independently. Existing monochrome API behavior is preserved.

## Measured results

- Displays: `led-lcd-srgb`, `led-lcd-wcg`, `oled`; ages: 24, 60, 60.001.
- Nine configurations: 10,080 resampled values and 216 raw integral/normalized
  matrix values, totaling **10,296 Float64 values with identical bits**.
- Test instrumentation captured 18 actual matrices from the official visual
  pathway. All match the independently calculated oracle bit for bit.
- Stock and instrumented official runs agree on identity Q/Q_JOD = 10 for
  32×32 three-channel white images. These identity runs check instrumentation;
  they do not validate nonidentity RGB quality prediction.
- Existing D65 regression: 2,520 samples plus 16,822 intermediate values across
  13 ages show zero numerical mismatches. The older checker uses numeric
  equality; this result does not assert signed-zero bit equality.
- Rebuilt public v4 luminance path: the historical 128×128 `competing_axes`
  pair passes 69 checks / 507,149 mixed Float32/Float64 bit comparisons.

Production calculations consume raw spectral files, not native reference
intermediates. Spectral processing remains on the CPU. MSVC builds use
`/fp:strict`; no shader or reference file was changed for this RGB checkpoint.

## Reproduction

Use fresh output directories. Native fixtures require the official installation
at `C:/HDR-VDP-3_Ref` and Octave image/statistics packages.

```powershell
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet tests/generate_rgb_spectral_reference.m validation/rgb_native_new
cmake -S tests/hdrvdp3_rgb -B validation/rgb_build_new -G 'Visual Studio 18 2026' -A x64
cmake --build validation/rgb_build_new --config Release
& validation/rgb_build_new/Release/rgb-spectral-test.exe validation/rgb_native_new C:/HDR-VDP-3_Ref/data validation/rgb_acceptance_new
```

The initial `native/` export failed because relocated instrumentation resolved
the data directory relative to itself. It is retained. The corrected `native_v2/`
explicitly points to the unchanged official data directory and is the accepted
export. Normal Octave exit-time `execution_exception` text accompanied exit 0.

## Remaining work

RGB is not yet an image-to-score implementation. Next capture nonidentity
`rgb-native` optical channel outputs and LMSR responses. Establish the exact
mixed Single/Double BLAS accumulation and coefficient conversion behavior,
then implement and verify the GPU projection and LMSR-to-JND entry.
Zero RGB inputs must be allowed before MTF filtering, with the official clamp
applied after filtering. Do not reuse the strict-positive monochrome input
contract by clamping RGB before FFT. RGB encodings/EOTF, PF image loading,
public runtime integration and full nonidentity reference comparisons remain.
