# Private96x96 image-to-score acceptance

Checkpoint: `validation/square_quality_639286340181932107` (2026-10-26).
The private synchronous in-memory API now processes original96x96 images
through visual processing, SPYR, pooling, comparison resizing, masking, CSF,
base-noise selection, perceptual differences and Q/Q_JOD. This is not yet
public CLI/runtime-package acceptance for96x96, arbitrary dimensions or RGB.

The quality diagnostic compiles private entry points under
`IQM_HDRVDP3_SQUARE_QUALITY_DIAGNOSTIC`, together with the preceding filter/SPYR
diagnostic definitions. Normal builds retain the original public names and
80x96/96x80 guards. Pool bands allow96,48,24,12 squares only in this profile;
comparison uses96x96 sources and native1/2/4/8 resize ratios. Base geometry is
144 elements, mean/DC290 words and FFT/magnitude/index433 words. Existing
source-derived sp0 coefficients and fourteen explicit compatible shader assets
are used; the private loader does not extend the public manifest format.

`evaluate_square_quality_diagnostic` receives only original image vectors,
dimensions, model/shader assets and an optional trace. It reads no image or
intermediate files. The checker loads all image-dependent goldens only after
the API returns scores. CPU model/filter/LUT preparation, double NCSF evaluation
from actual GPU-selected frequency/adaptation, and host-vector transfers remain,
as in the preceding accepted pathway. This does not claim fully GPU-resident
or CPU-free processing.

Native configuration is official HDR-VDP-3 v3.0.7, positive Float32 luminance,
30ppd,age24,D65/sp0, FFTW three threads/estimate with fresh single wisdom,
native192x192 padding, native DC processing and once-normalized inverse.
Layouts and signed-zero handling retain `SQUARE_VISUAL_CHAIN_VALIDATION.md`.
Fresh native case copies preserve the old traces and numerical exports;
`export_native_band_errors.m` computes exact official Minkowski errors and
asserts all captured accumulated errors on those fresh copies.

## Measured result

All six original image pairs pass69 checks each: **414 checks /1,711,662
Float32/Float64 values bitwise exact**, including signed zeros and the native
identity zero-frequency/+Inf-noise limit. No Vulkan validation errors.

| Content | Q | Q_JOD |
|---|---:|---:|
| Asymmetric | 3.9916315078735352 | 4.8269548416137695 |
| Impulse | 2.7713341712951660 | 3.4441552162170410 |
| Synthetic HDR ramp | 7.3951768875122070 | 8.2270421981811523 |
| Constant | 2.7713377475738525 | 3.4441585540771484 |
| Identity | 10 | 10 |
| Photograph-derived grayscale | 9.9999465942382812 | 9.9999980926513672 |

These are the same six pairs as the26/32/43-stage connected visual/SPYR
cohorts, not additional independent pairs. The photograph retains the explicit
assigned display-luminance mapping documented there, not captured scene
calibration or RGB validation. Native stock/traced scores agree before use.

The69 checks include both P maps, log adaptation, normalized bands, raw/self/
comparison masks, neighboring masks, denominators, differences, CSF, D,
masked/psychometric maps, band/total errors, final scores, actual base selection,
Float64 frequency/noise and base log/adaptation. Actual and expected binary
buffers plus first-mismatch diagnostics are retained. The earlier43-stage
cohort additionally checks visual FFT/filter maps and raw SPYR stages.

```powershell
cmake -S tests/hdrvdp3_residual144 -B <fresh-build> -G "Visual Studio 18 2026" -A x64
cmake --build <build> --config Release --target square-quality-test
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_evaluation/validate_square_quality.ps1 -Executable <build>/Release/square-quality-test.exe -AssetsDirectory <assets> -NativeDirectory <native-root> -OutputDirectory <fresh-results>
```

`-ResultsOnly` audits retained outputs. The native root contains six
`native_<case>/h96_w96` folders with original inputs, native maps and errors.
The private asset directory uses filenames from `asset_sources.json` in the
checkpoint: accepted square FFT/IO, mean144/difference144/residual144, source
runtime map/SPYR/pool/comparison/product, and the new source-generated base144
log-mean/power shader. Asset hashes are retained; none is an image-dependent
golden. `generate_base144_shader.ps1` uses the proven mean144 integer division
and existing native-style power helper.

Default production regression: the rebuilt public API passes both80x96 and
96x80 asymmetric tiny-contrast cases,69 checks /237,733 values each bitwise,
using the unchanged version1 runtime package. Public96x96 guards remain closed.
An initial private build omitted one explicit trace=false argument in its
forward declaration path; it was corrected before numerical execution. No
numerical tolerance was added and no quality mismatch was observed.

Next: design a distinct square96 runtime profile/package, source reproduction
and compatibility contracts, then expose the validated profile through the
public API/CLI and compare PFM inputs and exports. Broader dimensions, RGB,
viewing configurations and device coverage remain separate unfinished work.
