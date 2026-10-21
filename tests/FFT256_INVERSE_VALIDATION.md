# Isolated inverse256 Vulkan validation

The new experiment reproduces the native256 inverse graph without changing
the bounded32/64 quality API,its shader pins or runtime integration. Forward
FFT256 and full128x128 quality are still unimplemented.

The first axis uses scalar16/n1_16 leaves andt2_16 twiddles. The second uses
inverse8/n1_8 leaves andSIMD t3bv_32. Twiddle exponents are read from FFTW's
descriptor:1,3,9,15 for t2_16 and1,3,9,27 for t3bv_32. Source notices,
GPL-2.0-or-later license and archive/member/LF-hash provenance are preserved.

## Accepted results

18 fresh GPU executions compare **2,359,296 float components with zero numeric
differences**,without tolerance. Cases include MTF andLA filtered spectra from
the actual128-input native fixture,zero,off-center complex impulses,normal
random values at1e-3/1/1e3 scales,random subnormals at1e-40,andvalues around
the normal/subnormal boundary at1e-38. Synthetic inputs have independent
complex real/imaginary components. Native expected outputs are single ifft2.

The same18 saved GPU first-axis results independently match **2,359,296 float
components** from a native FFTW many-plan oracle. Its selected t2_16/n1_16
graph is checked before comparison. These first-axis checks reuse the same
GPU executions;they are not18 additional GPU runs. Signed-zero bit equality
is not asserted by the numeric float comparisons.

Input layout is column-major interleaved;complex output is explicitly converted
by the checker to the row-major native oracle layout. Both axes transpose
storage. Inverse normalization occurs once,at the final axis:1/65536.
The checker exports first-axis storage and final row-major values to fresh
directories. No Vulkan validation errors are reported on Intel Arc Pro Graphics.

Accepted SPIR-V explicitly requires DenormPreserve32 andRoundingModeRTE32.
The checker queries both device properties and rejects unsupported hardware.
Only Shader,DenormPreserve andRoundingModeRTE capabilities are declared;
Float64/Int64 andCPU result substitution are absent. Modes follow the
[Khronos SPIR-V environment](https://docs.vulkan.org/spec/latest/appendices/spirvenv.html).
Other hardware/toolchains have not been validated.

## Failed trials retained

- First shader used guessed twiddle exponents andfailed130,048/131,072
  components on the first MTF input. The descriptor-derived correction fixed
  this;no input-specific lookup or tolerance was introduced.
- With default float controls,14 end-to-end inverse cases initially matched
  1,835,008 components. Independent first-axis verification exposed21,088 LA
  differences,max1.504632769052528e-36. Small intermediates matter even when
  the final inverse rounds to the same result. That profile is not accepted
  as first-axis parity.
- Partial software denormal emulation attempts remained inexact andone
  generated candidate failed compilation. They are preserved as rejected
  artifacts andremoved from the committed generator. Device properties report
  preserve32=1,flush32=1,rte32=1. Explicit execution modes solve the measured
  first-axis issue;the seven accepted32/64 production reference shaders were
  not changed.

Evidence root:validation/fft256_inverse_639281620198383169;pointer
validation/fft256_inverse_completed_path.txt. COMPLETED.json/PASS.txt pin the
accepted preserve/RTE profile. PRESERVE_COMPLETED.json,preserve_suite/records.json,
axis0_preserve_results.json,accepted_source_hashes.json andaccepted_binary_hashes.json
retain detailed results. DEFAULT_CONTROLS_END_ONLY.json,ref.log,
axis0_la_diagnose.log andsoftware candidate directories preserve failures.
No old reference files were replaced.

## Reproduce and next step

Use tests/hdrvdp3_fft256/CMakeLists.txt,compile_inverse.ps1 andREADME.md.
Native synthetic fixtures come from generate_fft256_inverse_reference.m;
the CPU oracle is inspect_fftw_plan.cpp --inverse256-axis. Local drivers are
validation/run_fft256_inverse_preserve_suite.ps1 and
validation/check_fft256_inverse_preserve_axes.ps1;use fresh output directories.

Next:translate and independently verify the native real256 first forward axis
(hc2cf_32/r2cf_32/r2cfII_32/r2cf_8),then complex forward main/tail graphs.
After forward/inverse integration,extend residual FFT beyond8x8 andvalidate
128 resize/masking/reductions before widening the public metric API.
