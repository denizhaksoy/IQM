# Independent FFT64 reference tools

Build only the native64 FFT checker and Windows FFTW plan inspector,without
configuring/linking the HDR-VDP metric implementation:

```powershell
cmake -S tests/hdrvdp3_fft64 -B validation/fft64_tools_fresh
cmake --build validation/fft64_tools_fresh --config Release
```

Requires C++20,CMake3.20+,Vulkan SDK and a Vulkan device with validation layers.
The Windows inspector loads the existing Octave11.3.0 single FFTW DLL from
`C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin`;it does not distribute
that DLL. Native reference generation requires the local official HDR-VDP-3
v3.0.7 tree and the Octave image package.

Generate references and GLSL into fresh directories:

```powershell
octave-cli --quiet --no-gui tests/generate_fft_size_reference.m validation/native32_fresh 32
octave-cli --quiet --no-gui tests/prepare_fft_size_gpu_reference.m validation/native32_fresh validation/gpu64_fresh
powershell -NoProfile -ExecutionPolicy Bypass -File tests/generate_fft64_reference_shader.ps1 -OutputDirectory validation/shader64_fresh
glslc validation/shader64_fresh/fft64.comp -o validation/shader64_fresh/fft64.spv --target-env=vulkan1.2
spirv-val --target-env vulkan1.2 validation/shader64_fresh/fft64.spv
```

The shader generator requires the preserved official FFTW3.3.10 source archive
at `validation/fftw-3.3.10-inspection.tar.gz`,SHA256
`56C932549852CDDCFAFDAB3820B0200C7742675BE92179E59E6215B340E26467`.
It retains original codelets and COPYING in the output directory. Generated
shader/codelets are GPL-2.0-or-later and remain opt-in diagnostic artifacts.

Run both directions for both labels;each input contains exactly one64x64
complex matrix in column-major interleaved float order:

```powershell
./validation/fft64_tools_fresh/Release/fft64-reference-test.exe validation/gpu64_fresh/ref_forward validation/shader64_fresh/fft64.spv forward
./validation/fft64_tools_fresh/Release/fft64-reference-test.exe validation/gpu64_fresh/ref_inverse validation/shader64_fresh/fft64.spv inverse
# Repeat with test_forward/test_inverse.
./validation/fft64_tools_fresh/Release/fftw-plan-test.exe validation/native32_fresh validation/plans64_fresh 3 - 64
octave-cli --quiet --no-gui tests/validate_fft_size_plans.m validation/native32_fresh validation/plans64_fresh
```

The `-` means estimate without forcing native wisdom. Measured scalar/unaligned
plans match eight cases;aligned SIMD alternatives differ. Wisdom import fails
with a registry-header difference on this installation and is not claimed to
work. The GPU checker requires all8192 components to compare numerically
equal,with no tolerance. This does not test signed-zero bit identity or the
whole metric. See [full32 checkpoint](../FFT32_VALIDATION.md) for integration
scope and remaining work.
