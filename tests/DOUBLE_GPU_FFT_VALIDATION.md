# Initial Double GPU FFT validation

Validated2026-10-28. Completed root:
`validation/double_gpu_completed_path.txt`.

Vulkan capability inventory found Intel Arc Pro Graphics(driver101.8831)
with shaderFloat64=false and NVIDIA RTX A500 Laptop GPU(driver595.95) with
shaderFloat64=true. Both report RTE/signed-zero Float64 support; NVIDIA
reports denormPreserveFloat64=false. These tests exercise the finite optical/
adaptation/base-band inputs described below, not all Float64 value classes.

`Bin::VulkanInstance` now accepts optional preferred-device substring and
requireFloat64 arguments. Existing calls retain first suitable-device
selection and leave Float64 disabled. Only an explicit Float64 request enables
the feature; unsupported/missing matches are rejected before device creation.
No public CLI switch or sRGB routing was enabled. The device test confirmed
legacy Intel selection, explicit NVIDIA selection and two negative cases.

Two isolated GPU stages were added on NVIDIA, with Vulkan validation enabled:

- 12x12 real-forward base-band FFT, complex second axis, Hermitian completion,
DC suppression and wholly-real spectrum signed-zero storage behavior.
Three cases under three spectra passed864 Double component words each:
2,592 total. Inputs are native base-band-difference stage boundaries.
- The real192 first axis for192x192 padded optical/adaptation arrays. Each
case comprises24 matrices (three spectra,two sides,three optical channels
and one adaptation convolution),4,608 batched192-point transforms and893,952
Double component words. Three cases passed2,681,856 words total. This is an
isolated first-axis test, not the complete192x192 transform.

Cases are the existing stress and ramp distorted pairs plus an identity
control reusing the stress reference. Repeated identity content is not new
independent imagery. Original Single96x96/30ppd/age24/defaultsp0 profiles
produce the stage data; GPU inputs are Double column-major native stage
boundaries. First-axis outputs contain97 complex values per192-point column,
interleaved real/imaginary. Forward transforms are unnormalized. Optical
post-padding is symmetric and adaptation post-padding replicate; base-band
inputs are already normalized/DC-centered, and its FFT DC is then zeroed.

**2,684,448 GPU Double words passed exact binary comparisons**, including
signed zero; numerical tolerance is not used. Accepted directories are
`final_base_*` and `final_axis_*`. All Vulkan logs were checked without
reported validation errors. The normal IQM Release build succeeded and a
fresh legacy BT2020 OLED CLI evaluation on the default Intel path produced
all16 prior export files byte-identically.

Shader codelets are translated from hash-pinned FFTW3.3.10 ordinary scalar
sources; GPL-2.0-or-later notices/COPYING accompany generated diagnostics.
RTE64 and SignedZeroInfNanPreserve64 execution modes are explicit. The
matching192 twiddles are source-generated with reduced-angle Double sin/cos;
see the primary [FFTW trig implementation](https://github.com/FFTW/fftw3/blob/fftw-3.3.10/kernel/trig.c).
The helper uses local GNU Octave MinGW GCC15.2/libm at build time, not native
plan-memory or exported coefficient captures. It does not load FFTW to
generate coefficients. Final twiddle SHA256:
`7ef01b870c0b899b56e13ac9f73675a21dec9825fe1b7fc8827e6acac2a0849f`.

Higher precision was not equivalent: the initial Decimal coefficients gave
552,987 bit mismatches; extended-precision direct trig gave356,995; a split
table Double composition gave370,581. Direct reduced Double trig gave zero.
Untwiddled frequencies already matched in failed runs. These failed candidates
are preserved and not counted as exact passes. The original constant-array
packDouble2x32 GLSL initializer failed compilation and was replaced by exact
roundtrip Double literals. An initial test build used nonexistent submission
helpers; it was corrected to the existing Vulkan submit/fence API before any
acceptance run. The GCC helper required its bin directory on PATH.

Reproducible shader generation uses fresh output directories:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_double_gpu/compile_fft12_double.ps1 -OutputDirectory validation/new_fft12_build
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_double_gpu/compile_real192_double_axis.ps1 -OutputDirectory validation/new_real192_build
$r = Get-Content validation/double_gpu_completed_path.txt
node tests/hdrvdp3_evaluation/audit_double_gpu.cjs $r
```

Next port the native192 complex second-axis graph, then inverse axes and
normalization. Compare complete2D transforms/filtering before integrating
the remaining Double visual/quality stages into a production GPU/runtime
profile. The previously verified full CPU scores remain frozen; these GPU
stage passes do not establish full GPU scores or whole-metric parity.
