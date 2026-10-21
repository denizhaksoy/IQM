# Reference shader reproduction

The bounded32/64 luminance quality path now builds its reference shaders from
committed sources. The tested source commit is
`ffeed634246fca6530165c122cd065b8f65eaa54`. A fresh detached worktree and clean
VkFFT submodule built the API and regenerated all seven reference binaries
without the old FFTW archive, old binary assets, root build cache or downloads.

`tests/hdrvdp3_evaluation/generate_assets.ps1` runs four source generators,
compiles with glslc for Vulkan1.2 and runs spirv-val on every output. The
subsequent script update also checks REFERENCE_SHADER_HASHES.json and copies
licenses/toolchain metadata; rerunning it preserves all seven binary hashes.

| Runtime path | Purpose |
|---|---|
| fft32.spv |64-point transform for32-pixel images |
| fft64.spv |128-point transform for64-pixel images |
| base.spv |8x8 residual frequency selection |
| map.spv |ABI13 adaptation/JND/CSF/masking/difference/quality operations |
| math/prepare.spv |Reference preprocessing and padding |
| math/finish.spv |Reference reconstruction/crop/math |
| math/multiply.spv |Once-rounded complex/filter products |

FFTW3.3.10 original codelets and GPL-2.0-or-later notices are preserved. The
four additional runtime codelets have a source archive/member/LF-normalized
SHA256 manifest in tests/fftw_reference/runtime/SOURCES.json. The optional
FFTWArchive argument still validates the original archive hash. Existing
FFT128 source codelets retain their original attribution and COPYING.
AMD win-libm inputs retain the source commit, LF-normalized hash manifest
and MIT license in tests/math_reference. No Windows DLL or measured output
samples are used to generate the shaders. The numerical graphs target the
previously validated finite-domain native math behavior, not universal libm.

## Results

All7 regenerated SPIR-V hashes match the earlier accepted binaries exactly.
The clean build also regenerates7 embedded shader headers, all unchanged.
Two fresh direct API calls using the newly generated binaries preserve
returned error/Q/Q_JOD bits and203 numerical text export hashes (96 at32,
107 at64). Fourteen invalid API cases pass; sixteen poisoned environment
variables do not affect evaluation. Checkout and submodule remain clean.

These are hash comparisons against earlier native-validated baselines,
not additional fresh official-Octave/GPU-intermediate checker executions.
The scope is30ppd/age24/D65/photo-derived luminance. Spectral tables and
input PFMs are explicit external data; they were copied and hashed, not
generated. Input photometric calibration and RGB-native behavior are unchanged.

Generator guard checks reject a modified FFTW source and an existing output
directory without changing its old binary. All older artifacts are preserved.
An initial API launch occurred before its build finished and did not execute;
the verified retry uses assets_verified. This was not a numerical mismatch.

Evidence:validation/asset_generation_639281159480285385/COMPLETED.json;
comparison.json,clean_generate.log,configure.log,build.log,run32.log,run64.log,
generator_negative_checks.json,assets_verified/manifest.json and source worktree.
The final_manifest_check directory records the final pin/license/metadata update.
The local validation driver is validation/validate_generated_api.ps1.

## Reproduce

```powershell
git submodule update --init lib/VkFFT
cmake -S tests/hdrvdp3_evaluation -B <fresh-build>
cmake --build <fresh-build> --config Release -j 2
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_evaluation/generate_assets.ps1 -OutputDirectory <fresh-assets> -VulkanBin <VulkanSDK-Bin>
```

Use the explicit input/spectral paths with the direct caller or library API.
The validated toolchain is VulkanSDK1.4.363.0 on Windows/MSVC. Different binary
hashes require separate numerical validation before changing the pins.
General dimensions, full frames, RGB-native and other-platform parity remain open.
