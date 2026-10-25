# General-dimension frontier and GPU padding

Checkpoint: `validation/general_dimensions_639283336574679292`.
The public API/CLI dimension guards and all existing numerical engines remain
unchanged. This checkpoint adds independent native references and an isolated
GPU padding/layout implementation,not full GPU quality for these dimensions.

| Native input H x W | Required FFT H x W | Native residual H x W | Residual samples |
| --- | --- | --- | --- |
| 63 x 65 | 126 x 130 | 8 x 9 | 72 |
| 65 x 63 | 130 x 126 | 9 x 8 | 72 |
| 80 x 96 | 160 x 192 | 10 x 12 | 120 |
| 96 x 80 | 192 x 160 | 12 x 10 | 120 |
| 127 x 129 | 254 x 258 | 16 x 17 | 272 |
| 129 x 127 | 258 x 254 | 17 x 16 | 272 |
| 192 x 256 | 384 x 512 | 24 x 32 | 768 |
| 256 x 192 | 512 x 384 | 32 x 24 | 768 |

Official3.0.7 uses exact2H x2W post-padding,not rounding the FFT size up to a
power of two. Odd image dimensions also produce rounded-up SPYR band dimensions.
The native fixture generator records complete band geometry,FFT factorizations,
single FFTW wisdom and independent official Q/Q_JOD for eight positive asymmetric
reference/localized-test pairs at30ppd/age24. These native-only scores do not
establish GPU equivalence. PFM files are included in the local checkpoint.

`dimension_padding.comp` implements endpoint-inclusive symmetric MTF padding
and replicate LA padding,followed by explicit row-major or column-major complex
packing with positive-zero imaginary words. It copies uint representations of
Float32 values,so it performs no floating-point arithmetic and requires no
Float64/Int64 shader capabilities or floating-point execution modes. There is
no log/exp,filter multiplication,FFT,inverse scaling or DC processing in this
isolated stage. LA inputs here are already prepared scalar maps;this test does
not claim to implement the log stage.

All96 cases pass **14,843,520 bitwise comparisons**,including7,421,760 real and
7,421,760 positive-zero imaginary words. The real outputs include2,096 negative
zero words. Three input controls cover asymmetric positive luminance,signed
corner impulses/negative zero and an HDR ramp from1e-4 to1e5. Both padding modes
and layouts are tested at every shape. Native expected values use unmodified
Octave `padarray`;CPU code does not compute replacement GPU outputs.

The actual selected GPU in this run is **NVIDIA RTX A500 Laptop GPU**. Earlier
full-metric Intel Arc acceptance is historical and separate;this run extends
only integer-copy padding/layout coverage to this device. The generator uses
local Octave,FFTW3.3.10 single precision,3threads andestimate planning,with fresh
single wisdom before each independent full native quality pair.

```powershell
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet --no-gui tests/generate_general_dimension_reference.m <fresh-native-dir>
cmake -S tests/hdrvdp3_dimensions -B <fresh-build-dir>
cmake --build <fresh-build-dir> --config Release --target dimension-padding-test
& <fresh-build-dir>/Release/dimension-padding-test.exe <native-dir> <fresh-build-dir>/dimension_padding.spv <fresh-gpu-export>
```

The native generator requires the existing external official reference at
`C:/HDR-VDP-3_Ref` and Octave image/statistics packages. Only the new shader is
compiled;no blanket shader rebuild or deletion of previous binaries is used.
The GPU checker exports every compared word and per-case results.

Next target:80x96/96x80 at30ppd/age24. Determine exact native160x192/192x160
forward/inverse FFT plans,including strides,alignment,threading andoperation
ordering,then port those graphs. This also needs10x12/12x10 residual FFTs,
120-sample mean/DC reductions and map/resize contracts. Arbitrary-size masks,
prime-length FFTs (such as254),general public quality and RGB remain open.

## Subsequent experimental checkpoint (2026-10-25)

The next target above has since been completed as a file/host composed
experiment on Intel Arc Pro:80x96/96x80, three contents,30ppd/age24/D65.
Mixed FFTs,120-sample reductions,SPYR,resize/masking and final Q/Q_JOD now
pass the recorded native checks. See ACTUAL_GPU_COMPOSITION_VALIDATION.md
for exact scopes and artifacts. This does not broaden the historical NVIDIA
padding-only result or enable general public API geometries.
Production JND model tables and frequency-grid operation order have also
been corrected and individually checked for bit identity. The remaining
integration task is to provide these accepted mixed-axis/SPYR/residual stages
through the runtime API, preserving their dispatch,layout and DC contracts.
File transfers and CPU double NCSF remain in the experimental composition.
