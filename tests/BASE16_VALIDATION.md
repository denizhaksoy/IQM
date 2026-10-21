# Isolated16x16 residual FFT and first-max selection

Date:2026-10-21. This component checkpoint does not widen public API or
production MapGPU guards. Native128x128/30ppd SPYR requires a16x16 residual;
128x128/60ppd requires8x8, as measured in the filter-chain checkpoint.

Two GPU batch executions cover128 matrices. All65,536 raw FFT components,
32,768 Float32 magnitudes and128 selected indices match native Octave exactly,
without tolerance. Total checked scalar values:98,432. Numeric Float32
equality is checked;signed-zero bit identity is not asserted.

The first64 matrices contain eight controls (zero,constant,off-center impulse,
horizontal/vertical frequency,two-frequency mixture,alternating signs and
near-tie perturbation), plus56 seeded random matrices across scales2^-10..2^10.
The other64 matrices are official HDR-VDP-3 v3.0.7 sp0Filters residuals from
128x128 single inputs, decomposed at30ppd, normalized by get_band and mean/DC
processed by native Octave before export. Those source images are synthetic
JND-like fields,not full visual-pathway outputs. Their GPU mean/DC subtraction
is not tested here.

The shader uses original FFTW3.3.10 ordinary r2cf_16 and n1_16 scalar graphs,
separate Float32 multiply/add rounding, no normalization, and explicit
DenormPreserve32/RoundingModeRTE32. Sources, GPL notices and archive/member/LF
hashes are retained under `tests/fftw_reference/base16`. Three-thread native
wisdom records both codelets. A separate earlier installed-extension plan
probe reported22 threads;it is not the fixture's3-thread profile.

Input matrices are row-major interleaved real Float32 with zero imaginary
components. The first FFT axis processes logical columns, the second rows;
the missing half is expanded by Hermitian symmetry. Raw spectra are preserved.
Only the magnitude atDC is set tozero for selection. Magnitudes use the
existing integer-based correctly rounded Float32 finite_hypot helper, within
its documented normal exponent domain. Selection scans columns then rows and
updates only on a strictly greater magnitude, preserving Octave's first-index
tie behavior. Output indices are zero-based column-major,exactly represented
as Float32 integers0..255 for comparison. Zero andconstant cases select0.
There is no CPU substitution of GPU spectra,magnitudes or indices.

Reproduce using fresh directories:

```powershell
octave-cli --quiet --no-gui tests/generate_base16_reference.m <fresh-native>
octave-cli --quiet --no-gui tests/generate_base16_reference.m <fresh-spyr-native> spyr
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_fft256/compile_base16.ps1 -OutputDirectory <fresh-shader> -VulkanBin <SDK-Bin>
fft256-filter-test --base16 <fresh-native> <fresh-shader>/base16.spv <fresh-gpu>
fft256-filter-test --base16 <fresh-spyr-native> <fresh-shader>/base16.spv <fresh-spyr-gpu>
```

The shader writes per matrix512 spectrum components,256 DC-zero magnitudes
andone index (769 scalar values). The checker compares all values and exports
64x769 text arrays. Device:Intel Arc Pro Graphics. SPIR-V validation passes;
capabilities are Shader,DenormPreserve,RoundingModeRTE,without Float64/Int64.
Vulkan execution logs contain noERROR/Validation Error/VUID diagnostics.

Local evidence is recorded by `validation/base16_completed_path.txt`, with
native fixtures,wisdom,metadata,generated source/SPIR-V,logs,GPU exports,
hash records andCOMPLETED.json. Existing8x8 base FFT source/SPIR-V pins and
all production guards remain unchanged.

Remaining128 integration:ppd-limited pyramid height onboth evaluation SPYR
calls andband-count metadata;16x16 GPU mean/DC andbase adaptation;route base
FFT/first-max selection through MapGPU for16x16;128 map resize/masking and
pupil/quality reductions;full native stage/score regression before lifting
public API guards. Do not treat this isolated test as a completed128 metric.
