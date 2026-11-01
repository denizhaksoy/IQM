# LIVE HDR Single color and photoreceptor stages

2026-11-01. Scope: original-size 3840x2160 Daylight1 reference/test frame0,
local RGB48 PNG decode, clean official HDR-VDP-3 v3.0.7 CPU Octave baseline.
This extends the PQ preprocessing checkpoint; no independent Single quality
score or dataset equivalence is claimed.

## Added implementation

- `linear_color_single.h/.cpp`: absolute linear BT709/BT2020/XYZ to native
  display primaries. Independent existing Double spectral loader/calibration,
  Single coefficients at the matrix operation, explicit FMA order, gamut floor.
- `optical_response_single.h/.cpp`: already optically filtered native RGB to
  L/M/S/rod responses. Double spectral/age calibration, Single matrix arithmetic
  and clamping. Also supplies L+M adaptation input and natural log; these two
  auxiliary outputs have not yet been compared against captured official arrays.
- Both stages are in `IQM-HDRVDP3`; MSVC uses `/fp:strict`.

These APIs preserve planar row-major layout. They do not perform FFT filtering,
resize, JND conversion, pyramid construction, masking or score computation.

## Reference capture and transparency

`tests/capture_live_hdr_single.cjs` creates a fresh copy of clean v3.0.7 and
inserts calls to `tests/iqm_capture_single.m`. It verifies the original main
source SHA256 and unique insertion anchors. The clean tree stays unchanged.
The generated runner verifies the instrumented hash and asserts that the
captured run has the same Q/Q_JOD hex as the earlier unmodified stock run.

Run01 captures classes and full native RGB. Run02 additionally captures full
optical and clamped LMSR arrays. Both use the same hashed Single input tensors,
ppd62.729130063099447, age24, OLED, hdrvdp MTF, sp0Filters, symmetric optical
padding, CPU Octave11.3.0, FFTW3 threads / estimate. The pipeline's base-band
DC processing is unmodified. Captures are Octave column-major; C++ comparisons
explicitly translate to row-major and do not compare mismatched layouts.

The capture preserves Q=`40826c85`, Q_JOD=`409d6d04` (Single hex).
Known original-reference warnings remain in logs, as in the preceding checkpoint.

Observed precision at original size:

| Value | Class | Shape |
|---|---|---|
| BT2020-to-native matrix | Double | 3x3 |
| Native RGB | Single | 2160x3840x3 |
| Spatial frequency / MTF filter | Double | 4320x7680 |
| Optically filtered RGB | Single | 2160x3840x3 |
| Native-to-LMSR matrix | Double | 3x4 |
| Clamped LMSR | Single | 2160x3840x4 |
| Adaptation and achromatic response | Single | 2160x3840 |
| Base band before mean removal | Single | 135x240 |

## Numerical results

| Independent stage | Reference values | Test values | Bit differences / maximum ULP |
|---|---:|---:|---|
| BT2020 -> native RGB | 24,883,200 | 24,883,200 | 0 / 0 |
| Optical RGB -> clamped LMSR | 33,177,600 | 33,177,600 | 0 / 0 |

Total newly compared stage values: 116,121,600, all bitwise equal on this platform.
Color conversion computes from the independently generated linear tensors and
original spectral data; it does not inject a reference transform matrix.
LMSR validation deliberately uses the **official optical RGB as stage inputs**.
It establishes isolated spectral-stage parity, not independently reproduced FFT
output or end-to-end quality parity. Only age24/OLED/BT2020 is covered here.

## Artifacts and reproducibility

- `validation/live_hdr_capture_20261101_run01`: instrumented tree, generated
  runner, provenance, class trace and color comparison JSONs.
- `validation/live_hdr_capture_20261101_run02`: additional optical/LMSR arrays,
  class trace, stock-score comparison and optical comparison JSONs.
- Standalone tool sources: `tests/live_hdr_preprocessing/color.cpp`, `optical.cpp`.
- Built tools: `validation/live_hdr_single_20261101_run01/build/Release`.

Capture setup (use a new output path):

```powershell
node tests/capture_live_hdr_single.cjs validation/general_quality_639290762115433659/official_clean/hdrvdp-3.0.7 validation/live_hdr_single_20261101_run01 validation/live_hdr_frame_score_20261101_run01/score.json <fresh-capture-root>
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet <fresh-capture-root>/run_capture.m <fresh-capture-root>/instrumented validation/live_hdr_single_20261101_run01 <fresh-capture-root>/results
```

Standalone targets `live-color-single` and `live-optical-single` built successfully.
Main `cmake --build build-vs2026 --config Release --target IQM-HDRVDP3` passed;
existing glfw3 configuration warning remains. Existing shaders/VkFFT preserved.

Next: implement explicit Single FFT backend and independently reproduce the
optical arrays, preserving Double filters, Single transforms/products, padding,
normalization and crop. Then validate local adaptation before connecting the
already existing Single JND operations to a general RGB pathway.
