# Linear BT.709 conversion characterization

2026-10-27 checkpoint: `validation/rgb709_active_path.txt`.

Official HDR-VDP-3 v3.0.7/Octave11.3 evaluated three RGB96 linear709 pairs
using led-lcd-srgb,led-lcd-wcg,oled,30ppd,age24/default quality. Isolated
wrappers capture actual input, Float64 3×3 conversion matrix, Float32 raw
conversion and post-gamut-clamp input entering the visual pathway. Stock and
instrumented Q/Q_JOD bits agree for every pair. FFTW3 threads3/estimate with
cleared single wisdom; normal optical/adaptation padding and DC unchanged.
Original reference sources/data are not modified.

The native conversion is `M * RGB'`, preserves Single input precision here,
and clamps each result below1e-6 before optical filtering. Counts of clamped
values across reference/test: sRGB5210,WCG1488,OLED1520. Zeros and negative
out-of-gamut values are represented in this fixture; the clamp cannot be omitted.

## Arithmetic evidence

Six images /165,888 Float32 raw conversion values:

| Candidate | Bit mismatches |
| --- | ---: |
| Separate left-associated products/sums | 26,589 |
| Separate right-associated products/sums | 45,281 |
| Float32 coefficients, `fma(B,b,fma(G,g,R*r))` | **0** |
| Reverse FMA order | 51,866 |
| Double coefficients/accumulation, final Float32 cast | 46,801 |

`hdrvdp3_rgb709_projection.comp` follows the observed forward order and emits
raw and clamped planes. GPU comparison passes **331,776 Float32 words bitwise**
(165,888 raw plus165,888 clamped) on Intel Arc Pro with Vulkan validation.
No validation errors were reported. SPIR-V passes spirv-val and contains one
NoContraction FMul followed by two explicit Fma instructions.

This is conversion-stage acceptance **using native-captured matrix coefficients**.
It does not validate independent matrix generation, an exposed709 API/CLI,
or GPU-produced final709 quality scores. Current production CLI still rejects
rgb-bt.709. Existing rgb-native code, shaders and runtime pins remain unchanged.

## Reproduction and next step

Generate fresh captures with `generate_rgb709_reference.m <fresh>`. Build
`rgb709-projection-inspect` in `tests/hdrvdp3_rgb` and
`rgb709-projection-gpu-test` in `tests/hdrvdp3_residual144`. Compile the new
projection shader individually with glslangValidator; validate with spirv-val.
Both tools take native directory and fresh output; the GPU tool additionally
takes the shader path as its second argument.

Next load/resample original ciexyz31.csv with the native420-point grid, compute
the display-primary XYZ integrals and normalized matrix independently, then
match the3×3 solve. Native integration scales by683.002 **after trapz**, unlike
the accepted LMSR calibration's per-sample scaling. Preserve native Y-row sum
order and characterize LAPACK pivot/division/FMA ordering if the solve differs.
Only then compose converted originals with the accepted native96 quality path
and expose the new encoding after full-score validation.

sRGB-display remains a separate task: its native display model creates Double
output. It cannot be implemented by silently rounding into this Single path.
