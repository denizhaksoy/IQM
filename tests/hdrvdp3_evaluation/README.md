# Isolated bounded evaluation build

This project builds the local `evaluate_quality` library and a direct caller.
It does not configure other metrics, download dependencies, use the main
build cache, or read/replace the workspace's existing `shaders_out` headers.
Seven embedded shader headers are generated from GLSL into the new build
directory, including both corrDn arithmetic variants.

Prerequisites: C++20 compiler, CMake3.29+, Vulkan SDK with glslc and compiler
libraries, and the existing VkFFT headers. VkFFT is still compiled as a legacy
alternative inside LocalAdaptFFT; numerical validation uses explicit native
reference FFT assets. The dirty VkFFT checkout must be preserved. Reference
FFT/math/map/baseFFT assets are explicit runtime inputs, now generated from
committed sources by the accompanying script. Raw D65 data remains an external
input. FFTW-derived code retains GPL-2.0-or-later notices; AMD math sources
retain their MIT license. Licenses are copied into generated assets.

```powershell
cmake -S tests/hdrvdp3_evaluation -B validation/<fresh-build>
cmake --build validation/<fresh-build> --config Release -j 2
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_evaluation/generate_assets.ps1 -OutputDirectory validation/<fresh-assets>
```

`Release/IQM-quality-api-test.exe` accepts the arguments documented in
`tests/evaluate_quality_api.cpp`. See `tests/EVALUATION_API_VALIDATION.md` for
the numeric contract and validation scope. The bounded source set has now been
committed and built from a clean worktree with a clean VkFFT submodule;32/64
direct evaluations match the pinned baseline. See
`tests/CLEAN_CHECKOUT_VALIDATION.md` and `tests/REFERENCE_ASSET_VALIDATION.md`.
The generator does not require the old local FFTW archive or prebuilt shader
binaries. glslc is found through VULKAN_SDK/PATH,or explicit -VulkanBin. Generated
binary hashes must match REFERENCE_SHADER_HASHES.json;different toolchains need
separate numerical validation before changing those pins.

Generated paths:fft32.spv is the64-point FFT for32-pixel inputs;fft64.spv is
the128-point FFT for64-pixel inputs. map.spv/base.spv andmath/prepare.spv,
math/finish.spv,math/multiply.spv complete the seven runtime shaders. The build
separately regenerates seven embedded headers. Spectral CSVs and input images
are explicit external data,not generated shader dependencies. Only the bounded
32/64 luminance domain is verified;this is not general HDR input support.
