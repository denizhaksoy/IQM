# Isolated FFT256 experiments

This project evaluates inverse256 independently of the bounded quality API.
It does not enable full128x128 quality evaluation in the public API.

Forward generation is available using the same compile script with `-Forward`.
The checker accepts `--forward` for two axes or `--forward-axis` for the first
axis only. Forward fixture names are `ref_mtf_stage0.txt` and
`ref_native_forward.txt` (or test equivalents); first-axis mode instead expects
`ref_axis0_compact.txt`. Both full input and expected files contain256x512
scalar text values; the compact intermediate contains256x258. Generate fresh
synthetic native references with `tests/generate_fft256_forward_reference.m`.

The first forward axis applies32 real8 leaves into halfcomplex scratch, then
real32 for DC, hc2cf_32 for paired frequencies1/7,2/6,3/5, and real-II32 for
frequency4. Its output is256 columns of129 interleaved complex frequencies.
The second axis uses16 forward16 leaves and t3fv_16 SIMD twiddles, then expands
the Hermitian half. There is no normalization, DC removal or CPU substitution.
Final GPU storage is column-major; the checker exports row-major for Octave.
See `tests/FFT256_FORWARD_VALIDATION.md` for measured scope.

The `fft256-filter-test` target directly checks128x128 MTF/LA filter chains
without changing public API guards. Its CLI takes native fixture,forward SPIR-V,
inverse SPIR-V,math directory,ref|test,mtf|la andfresh export. The four-case
`validate_filter.ps1` driver saves logs,counts and provenance. See
`tests/FFT256_FILTER_VALIDATION.md` for measured chains and the16x16 residual
required by official128x128/30ppd SPYR height control.

`fft256-filter-test --base16 <fixture> <shader> <fresh-output>` checks the
isolated residual16 FFT,magnitudes andfirst-max indices. Generate fixtures
using `tests/generate_base16_reference.m` (optional `spyr` input source) and
compile using `compile_base16.ps1`. See `tests/BASE16_VALIDATION.md`;production
MapGPU base16 routing/DC/adaptation are still separate integration work.

The first native inverse axis uses sixteen scalar16 leaves followed by t2_16.
The second uses32 inverse8 leaves followed by t3bv_32 SIMD twiddles. Original
FFTW3.3.10 source and GPL-2.0-or-later attribution/COPYING are preserved.
The added codelets have archive/member/LF-normalized hash provenance in
tests/fftw_reference/fft256/SOURCES.json. Other leaves reuse the existing
source-backed FFT128 generator. Twiddle powers are parsed from the original
codelet descriptor,not inferred from transform length.

```powershell
cmake -S tests/hdrvdp3_fft256 -B <fresh-build>
cmake --build <fresh-build> --config Release -j 2
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_fft256/compile_inverse.ps1 -OutputDirectory <fresh-shader> -VulkanBin <SDK-Bin>
```

The compiler preserves the GLSL graph and explicitly adds DenormPreserve32
and RoundingModeRTE32 execution modes to SPIR-V,then validates the module.
Original/default-control SPIR-V is retained. The checker queries the device's
Float32 control properties and rejects unsupported devices before dispatch.
These modes are defined in the [Khronos Vulkan SPIR-V environment](https://docs.vulkan.org/spec/latest/appendices/spirvenv.html).
No Float64/Int64 capability or CPU output substitution is used.

Input is a256x256 complex matrix stored column-major/interleaved. Both axes
transpose storage;normalization occurs once at the final axis,1/65536.
The checker explicitly converts final GPU storage to the native row-major
expected layout. It saves the first-axis storage and final row-major output.
Fresh export directories are mandatory.

The checker accepts native-fixture directory,shader,ref|test andfresh export:
`fft256-inverse-test <fixture> <shader.spv> ref <fresh-output>`.
The fixture filenames are ref_mtf_stage2.txt andref_native_inverse.txt (or
test equivalents). Synthetic complex matrices use these compatibility names;
their metadata explicitly identifies them as inverse fixtures,not MTF data.
`generate_fft256_inverse_reference.m` includes zero,complex impulse,normal
random scales,subnormal random values andthe normal/subnormal boundary.

The native DLL inspector's --inverse256-axis mode independently verifies the
saved GPU first-axis output. It checks the selected many-plan fingerprint
contains t2_16/n1_16 andnever substitutes its CPU results for GPU outputs.
See tests/FFT256_INVERSE_VALIDATION.md for completed results and failed trials.
