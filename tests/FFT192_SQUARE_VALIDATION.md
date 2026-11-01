# Isolated192x192 optical/adaptation FFT acceptance

Checkpoint: `validation/fft192square_639286146863661089` (2026-10-26).
This proves the tested FFT stages for the experimental96x96 geometry. It does
not enable96x96 in the public runtime or establish a complete visual pathway.

`generate_rectangular_filter_reference.m` now permits96x96 under `--mixed`.
Its new MAT-trace fallback applies only to that square geometry; historical
rectangular stage-file loading remains unchanged. Three source cases use the
previously accepted native96x96 visual traces in the mean144 checkpoint:
asymmetric, impulse and synthetic HDR ramp, each reference/test. Native retinal,
L+M adaptation input and final adaptation must match those full visual traces
before staged FFT values are exported. This is not real-HDR dataset acceptance.

Contract: official HDR-VDP-3 v3.0.7, positive Float32 original luminance,
30ppd/age24/D65, Float32 transforms and double filter arrays with native mixed
precision promotion, 192x192 padding without power-of-two rounding, FFTW three
threads/estimate with fresh single wisdom. MTF uses symmetric post-padding;
local adaptation uses replicate post-padding after the official log operation.
FFT DC is retained. Forward is unnormalized; inverse divides once by36864.
Input/stage2 exports are column-major interleaved complex values; native final
forward/inverse exports are row-major. Exporters extract entire real/imaginary
arrays, preserving complex signed-zero bits.

The CPU plan census examines32 combinations across reference/test, MTF/LA,
three/one thread, aligned/unaligned and in/out-of-place. Eight fresh unaligned
out-of-place variants match forward and inverse numerically. Imported wisdom
does not directly supply those matching candidates. This census alone is not
exact-bit parity. Rounded reciprocal multiplication differs from native division
on7,706–8,239 components in the four asymmetric inverse outputs.

The new generator composes existing source-derived real192/scalar inverse192
first axes with the already validated complex192 SIMD graph translations from
the160x192 experiments. It preserves codelet operation ordering, explicit FMA
boundaries, twiddles, Hermitian expansion and transposed storage. Compilation
adds Float32 DenormPreserve/RTE modes and validates SPIR-V. Existing binaries
and runtime pins remain unchanged. The inverse normalization source also has
a standalone integer diagnostic entry, enabled only by a compile definition;
regeneration from the final source reproduces the accepted inverse SPIR-V
bytes exactly (`inverse_source_reproduction/PASS.txt`).

`square192_normalization.comp` implements correctly rounded finite Float32
division by36864=9×4096 with integer significand division and remainder-based
ties-to-even. Normal, subnormal and signed-zero branches preserve the sign;
the significand threshold is9×2²⁰, with normal exponent offsets15/16. The
standalone shader matches CPU Float32 division on1,054,696 deterministic
finite IEEE probes, including exponent/fraction boundaries and signed zeros,
plus294,912 native inverse components from four asymmetric chains. These
native normalization components overlap the staged inverse acceptance and
are not additional unique images. The probes do not claim exhaustive coverage
of all Float32 bit patterns.

## Measured acceptance

`validate_fft192square.ps1` runs3 contents ×2 images ×2 chains ×2 directions:
**24 runs /1,769,472 Float32 words bitwise exact**, including signed zeros.
Every run compares the complete complex output, not only a real crop. All
process exits are zero and no Vulkan validation errors are reported. Outputs,
first-axis dumps, binary word dumps, logs, per-case strict reports and hashes
are retained under `gpu_matrix`. This uses original native prepared FFT input
or native filtered spectrum as isolated stage inputs; it is not yet a connected
GPU optical/adaptation chain.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_fft256/compile_fft192square.ps1 -OutputDirectory <fresh-forward> -Direction forward
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_fft256/compile_fft192square.ps1 -OutputDirectory <fresh-inverse> -Direction inverse
cmake -S tests/hdrvdp3_residual144 -B <fresh-build> -G "Visual Studio 18 2026" -A x64
cmake --build <build> --config Release --target square192-fft-test square192-normalization-test square-native-plan-test
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_evaluation/validate_fft192square.ps1 -Executable <build>/Release/square192-fft-test.exe -ForwardShader <forward>/forward192square.spv -InverseShader <inverse>/inverse192square.spv -NativeDirectory <native-root> -OutputDirectory <fresh-results>
```

The validator's `-ResultsOnly` option audits retained outputs. Native roots
need `native_asymmetric`, `native_impulse`, `native_hdr_ramp`, generated using
`generate_rectangular_filter_reference.m <fresh> 96 96 <pattern> --mixed
--pathway <native-visual-case>`. The standalone normalization entry compiles
with `glslc -DIQM_NORMALIZATION_STANDALONE square192_normalization.comp`; its
caller accepts `--square192` and the retained four-pair normalization manifest.

Regression checks:27 old80x96 native staged text files are byte-identical;
the shared FFT caller still passes the original160x192 forward/inverse runs;
the old30720 normalization shader still passes1,054,696 synthetic and1,474,560
native words bitwise. Those FFT legacy runs retain their historical numerical
acceptance scope. No production code or public geometry guard changed.

Next: connect192x192 preparation, FFT/filter product, normalized inverse and
crop/exp without native image-dependent intermediate inputs, then integrate
the96x96 SPYR and144-element base layouts. Keep public96x96 support disabled
until P/log, all bands/masks/errors and final scores pass end-to-end.
