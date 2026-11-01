# RGB optical-to-LMSR projection

2026-10-27 checkpoint: `validation/rgb_projection_639287155801831764`.

Three nonidentity 96×96 `rgb-native` pairs were evaluated with official
HDR-VDP-3 v3.0.7 / Octave 11.3, age 24, 30 ppd, and display spectra
`led-lcd-srgb`, `led-lcd-wcg`, `oled`. Inputs are planar column-major
Float32 RGB ramps with a zero-valued patch, pure-primary patch and localized
red/blue distortions. No input clamp, gamma conversion or resizing is used.
Native MTF uses its normal symmetric padding and retains DC.

Isolated instrumentation copies capture optical RGB **after the native MTF
clamp**, the Float64 3×4 spectral matrix, and LMSR **before** its response
clamp. Official sources/data remain unchanged. Stock/instrumented Q and Q_JOD
bits agree for all three pairs; native scores are recorded in `metadata.json`.
These scores are instrumentation checks, not scores produced by the GPU test.

## Arithmetic evidence

Across six captured images / 221,184 Float32 response values:

| Candidate | Bit mismatches |
| --- | ---: |
| Separate `(R*r + G*g) + B*b` | 46,608 |
| Separate `R*r + (G*g + B*b)` | 66,247 |
| `fma(B,b,fma(G,g,R*r))`, Float32 coefficients | **0** |
| Reverse FMA order | 67,204 |
| Float64 coefficients/accumulation, final Float32 cast | 66,989 |

This characterizes the current native BLAS environment; it is not a claim
about all MATLAB/Octave/BLAS builds. The Vulkan projection shader uses the
measured forward FMA order. SPIR-V inspection confirms one `OpFMul` marked
`NoContraction` followed by two explicit GLSL `Fma` instructions.

`rgb-projection-gpu-test` passed **221,184 Float32 words bitwise** on Intel
Arc Pro, with Vulkan validation enabled and no reported validation errors.
It reads captured native optical inputs and matrix coefficients, casts each
matrix coefficient once to Float32, and dispatches the shader. Output layout
is four column-major planes. Float64 GPU arithmetic is not required.

## Reproduction

Use fresh output directories; native fixtures require the official installation
at `C:/HDR-VDP-3_Ref` and Octave image/statistics packages.

```powershell
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet tests/generate_rgb_projection_reference.m validation/rgb_projection_native_new
cmake -S tests/hdrvdp3_rgb -B validation/rgb_projection_cpu_build -G 'Visual Studio 18 2026' -A x64
cmake --build validation/rgb_projection_cpu_build --config Release --target rgb-projection-inspect
& validation/rgb_projection_cpu_build/Release/rgb-projection-inspect.exe validation/rgb_projection_native_new validation/rgb_projection_arithmetic_new
cmake -S tests/hdrvdp3_residual144 -B validation/rgb_projection_gpu_build -G 'Visual Studio 18 2026' -A x64
cmake --build validation/rgb_projection_gpu_build --config Release --target rgb-projection-gpu-test
& C:/VulkanSDK/1.4.363.0/Bin/glslangValidator.exe -V --target-env vulkan1.2 shaders/hdrvdp3/hdrvdp3_rgb_projection.comp -o validation/rgb_projection_new.spv
& C:/VulkanSDK/1.4.363.0/Bin/spirv-val.exe --target-env vulkan1.2 validation/rgb_projection_new.spv
& validation/rgb_projection_gpu_build/Release/rgb-projection-gpu-test.exe validation/rgb_projection_native_new validation/rgb_projection_new.spv validation/rgb_projection_gpu_new
```

## Remaining scope

This is projection-only GPU acceptance using reference optical intermediates.
It does not establish independent RGB optical filtering, JND conversion,
RGB image-to-score parity or public RGB runtime support. Next implement an
explicit nonnegative RGB optical-input mode (clamp after filtering), compare
each channel with these captured optical outputs, then connect LMSR response
clamp, adaptation/pupil and JND while retaining the monochrome contract.
