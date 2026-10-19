# 32x32 full metric reference checkpoint

Updated2026-10-19 (Europe/Prague). Local integration only;public API remains
incomplete. This checkpoint extends the previous64x64 scope using the opt-in
reference FFT/math and Vulkan map backends. No reference files were modified.

## Measured results

Six32x32 pairs andsix64x64 regressions pass strict preprocessing,JND,CSF,
raw/DC-centered SPYR,masking,raw/final D,base noise,bitwise comparison masks
and direct official HDR-VDP-3 v3.0.7 Q/Q_JOD comparison.

|32x32 pair|Q|Q_JOD|
|---|---:|---:|
|asymmetric_localized|8.3328084945678711|8.9990530014038086|
|asymmetric_stronger|9.9952077865600586|9.9994449615478516|
|asymmetric_brighter|9.9999408721923828|9.9999980926513672|
|diagonal_brighter|9.9999361038208008|9.9999980926513672|
|impulse_brighter|10|10|
|asymmetric_identical|10|10|

All scores equal the direct official call. Per32x32 pair,the independent GPU
checker compares62 float cases/57046 values andfour double masks/2368 values.
All six pairs:342276 float comparisons and14208 binary64 mask comparisons,
zero mismatches. The64x64 regressions add1368330 float values and57216 double
mask values,zero mismatches. Vulkan application/checker logs contain no
validation errors/VUIDs. These counts are comparisons,not coverage percentages.

## FFT and preprocessing

At30ppd,32x32 has height2 andfour SPYR bands:32x32,32x32,16x16,8x8. Base FFT/DC
therefore retains the existing8x8 implementation;physical base sampling is
7.5ppd rather than3.75ppd. Input visual area is32*32/(30*30);age24,D65 and
positive synthetic luminance are unchanged.

The new64x64 FFT shader translates scalar FFTW3.3.10 `r2cf_64` and `n1_64`
graphs,with separate product/add rounding. ABI6401 uses the existing two-pass
graph selectors8/9 forward and3/4 inverse. Buffers are column-major interleaved
binary32;inverse normalization divides by4096 once. Four isolated transforms
(two images,forward/inverse) compare32768 components exactly. Shader SHA256:
`8F339ADF6FF03BB5E1D44EF1D767682ABDF136C7E82EF1C9FD30C719CAAD5460`.
Only Shader capability is present;no shaderFloat64/shaderInt64 requirement.
Generated FFTW source files,notices and COPYING remain with the opt-in shader.

Independent inspection of the installed Octave FFTW DLL found scalar/unaligned
plans match all eight native forward/inverse comparisons (one/three threads,
two inputs). Eight aligned AVX2 comparisons differ. Native wisdom import
failed with a registry-header checksum difference despite the same loaded DLL
path/version;it was not forced or rewritten. The scalar path was accepted by
direct numerical comparison,not by claiming a successful wisdom import.
The cause of the checksum difference remains unresolved.

LocalAdaptFFTInput now carries square input dimensions with64 defaults. The
reference backend accepts32/64;32 with VkFFT is explicitly rejected. FFT scratch,
dispatch,crop,trace buffers and kernel exports use actual dimensions.32 input
uses64 FFT;64 input retains128 FFT. Dimensions cannot change after initialization.

MTF uses symmetric post padding;local adaptation uses replicate post padding
after native log clamp. Gaussian sigma is10^-0.781367*30. Both consume the
same exported binary32 luminance and native L+M coefficients. Float traces are
compared numerically after round-trip serialization;double masks use bit
comparison. Isolated MTF/local-adaptation checks cover asymmetric,impulse and
10^-3..10^4 cd/m2 ramp inputs at32,plus the existing asymmetric64 path:
1075200 float comparisons,zero differences. Native staged outputs are also
asserted equal to official fast_conv_fft/hdrvdp_local_adapt results.

## Reproduction and evidence

The [independent FFT64 project](hdrvdp3_fft64/README.md) builds the checker and
native plan inspector without the mixed metric integration. It is verified
against the same four GPU FFT fixtures. The full metric command below still
requires the local application/integration build.

The local full test harness accepts IQM_SPYR_SIZE=32/64. Validators discover
band_count.txt (four/five);older exports without it retain five-band behavior.
The quality runner accepts-InputSize and-GPUChecker.32 runs use a separate
prefix and IQM_FFT_REFERENCE_SHADER64,avoiding accidental use of the128 shader.
It clears isolated probe flags before running the full metric.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/run_quality_validation.ps1 -InputSize 32 -ReferenceShader validation/fft32_native_639280099461894780/shader64/fft64.spv -MathShaderDirectory validation/source_math_exact_division_639278581939061846 -MapShader validation/mask_resize_gpu_639279174670035352/map.spv -BaseFFTShader validation/map_gpu_abi8_639278691970167183/base_fft_abi81/base_fft.spv -GPUChecker validation/fft32_native_639280099461894780/checker32_build/Release/IQM-map-gpu-test.exe
```

Aggregate evidence:validation/fft32_native_639280099461894780/COMPLETED.json,
PASS.txt,full_suite_paths.json,source/binary hashes and saved executables.
fft32_completed_path.txt selects it. Initial failed wisdom imports and the
first32 checker failure are retained. The first32 application result already
matched official Q/Q_JOD;the checker still requested literal csf_log_band4.txt.
That filename was corrected and all six cases rerun in a fresh suite using a
separate checker build,without changing the concurrent64 regression binary.

## Remaining scope

This proves six synthetic32 pairs at30ppd/age24/D65,not arbitrary dimensions,
viewing parameters,color inputs,devices or output modes. The main32 test
wrapper still has64-specific HDR ramp scaling/impulse coordinates;correct
those before claiming equivalent full-HDR32 or three-impulse metric coverage.
The separate preprocessing HDR/impulse generator already uses size-aware
coordinates/range. Add32 low-quality/threshold comparisons next,then continue
viewing parameters,real HDR/color,public API and remaining CPU preparation.
