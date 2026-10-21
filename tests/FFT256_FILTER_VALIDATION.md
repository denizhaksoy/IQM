# Isolated128x128 MTF and local-adaptation chains

Date:2026-10-21. Public metric API guards remain32/64. This checkpoint verifies
filter chains, not full128x128 HDR-VDP-3 scores or production integration.

Twelve GPU chains match native Octave single-precision reference outputs in
6,488,064 float components, with zero numeric mismatches and no tolerance.
Inputs cover asymmetric spatial luminance, off-center impulses and an HDR
ramp from1e-3 to1e4 cd/m2, each with ref/test and MTF/LA modes. For each run,
prepared values, forward spectra, filtered spectra, inverse values and final
cropped images are checked independently: four131,072-component complex
exports and one16,384-pixel final image. Signed-zero bit identity is not asserted.

The actual GPU chain uses R32_SFLOAT storage images, committed reference math
prepare/multiply/finish shaders, and the separately validated forward/inverse
FFT256 shaders. It uses no CPU stage-output substitution. Inputs are row-major
positive luminance or native L+M adaptation-input images. MTF uses symmetric
post-padding and final clamp[1e-5,1e10]; LA uses native log(max(input,1e-6)),
replicate post-padding, and final native exp. FFT buffers are column-major;
spectra/inverse exports are explicitly reordered for row-major comparisons.
Inverse normalization occurs once at1/65536. No DC removal is applied here.

CPU-generated MTF and Gaussian filters match the native filters after rounding
toFloat32, in786,432 comparisons across the12 runs. This does not assert
binary64 filter identity. The diagnostic uses the existing production MTF
generator and the same double Gaussian formula as LocalAdaptFFT, at30ppd.
The rounded generated filters, not native reference buffers, are uploaded.
Reference generation calls official HDR-VDP-3 v3.0.7 MTF/local-adaptation
functions and asserts their staged outputs match the official functions.

The test calls individual runtime shaders directly. It does not bypass or
relax public dimension guards and does not verify LocalAdaptFFT's production
dispatch or API integration. MTF and LA cases take their own native inputs;
this is not a continuous visual-pathway/pupil/JND/SPYR/quality run.

Device:Intel Arc Pro Graphics. Explicit DenormPreserve32 andRTE32 required for
the FFT shaders. No Float64/Int64 shader capability is introduced. Vulkan
validation logs contain noERROR/Validation Error/VUID diagnostics.

Build target `fft256-filter-test` in `tests/hdrvdp3_fft256`. A reproducible
four-case driver is `tests/hdrvdp3_fft256/validate_filter.ps1`; it accepts the
executable, native fixture, forward/inverse SPIR-V, math directory and fresh
output directory. Generate fresh128x128 fixtures with
`tests/generate_fft_size_reference.m <fresh-native> 128 C:/HDR-VDP-3_Ref <pattern>`.
Patterns are asymmetric, impulse andhdr_ramp. Keep all old directories.

Evidence: `validation/fft256_filter_639281807624298950`, `records.json`,
per-case logs and five numeric exports per run. Asymmetric fixtures reuse
`validation/fft256_frontier_639281615622631080/native128`; impulse andHDR
fixtures are newly generated under this checkpoint. Binary/source/input hash
records and completed scope are saved beside the logs.
Four final-source asymmetric driver replays also pass;they are retained in
`driver_replay` and excluded from the12-case unique-input counts above.

Native pyramid inspection also confirms a required integration detail:

|128x128 input|Official SPYR height|Band sides|Residual|
|---|---:|---|---:|
|30ppd|3|128,128,64,32,16|16x16|
|60ppd|4|128,128,64,32,16,8|8x8|

Official `hdrvdp_spyr.decompose` limits height by both
`ceil(log2(ppd))-2` and `maxPyrHt`. The current bounded evaluation wrapper
uses the maximum size-based height, which is equivalent for its validated
32/64 inputs. Extending it to128 requires supplying the ppd-limited height
to both SPYR calls and band-count metadata. At30ppd this also requires a
validated16x16 residual FFT and dominant-frequency selection; existing base
GPU paths accept only8x8. The inspected native16x16 FFT uses r2cf_16/n1_16;
the separate extension plan probe reports22 threads and is diagnostic only,
not part of the3-thread FFT256 validation counts.

Next:validate residual16x16 FFT/selection and pyramid height control, then
128 map resize/masking/reductions before lifting dimension guards or claiming
full native128x128 metric parity.
