# Double192 GPU inverse validation

Validated 2026-10-29. Artifact pointer: `validation/double192_inverse_completed_path.txt`.

Both inverse axes run consecutively on NVIDIA RTX A500 Laptop GPU using
Float64. Input is the official Double filtered spectrum at the convolution
boundary, not original images. The target is the actual `real(ifft2(...))`
operation. No CPU intermediate is supplied between GPU axes.

Three cases (two distorted image pairs and an identity control reusing the
stress reference), three spectra, two sides and four convolutions per side
cover 72 padded 192x192 matrices. Original profile: 96x96,30ppd,age24,
defaultsp0; symmetric optical and replicate adaptation postpadding; DC unchanged.

Results:
- Complete normalized real inverse: 2,654,208 Double words, zero bit errors.
- Isolated normalized first axis: 5,308,416 complex component words, zero bit
  errors. This is overlapping coverage, not additional images.
- Complete forward regression with updated harness: 1,769,472 Double words exact.
- Fresh Octave inverse expectations byte-equal the previous official native
  convolution inverse captures. All accepted Vulkan logs have no VUID/[ERROR].

The first inverse axis uses ordinary t2_16/n1_12 graphs, a transposed temporary
and 96 source-generated Double twiddle words. The second uses t3bv_8/t1bv_12/
n1_2 FMA graphs and 188 complex twiddle words. Official FFTW3.3.10 sources are
hash-pinned in `fftw_reference/double192_inverse`, with GPL COPYING retained.
All old references and failed variants are preserved.

Plain GPU division caused 591,041 first-axis component mismatches and 333,196
final real mismatches in the initial case. Normalization now computes q=a/d,
r=fma(-q,d,a), and q+r/d, preserving signed zero explicitly. This eliminated
all observed mismatches for divisors192 and36864. It is an empirical result
for the tested profile, not a proof of correct rounding for every Double input.
SPIR-V explicitly requests RTE64 and SignedZeroInfNanPreserve64. The device
lacks denormPreserveFloat64; arbitrary subnormal behavior is not validated.

Reproduction (fresh directories required):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_double_gpu/compile_inverse192_double.ps1 -OutputDirectory <fresh-pack>
# Capture each case from its preserved official convolution directory:
octave-cli --quiet tests/capture_double192_inverse.m <fresh-native> <official-native>
# Existing diagnostic CMake target: tests/hdrvdp3_double_gpu / fft12-double-gpu-test
fft12-double-gpu-test <fresh-native> <fresh-pack>/first/inverse192_double_axis.spv <fresh-output> --inverse192 <fresh-pack>/second/complex192_double_inverse.spv NVIDIA
node tests/hdrvdp3_evaluation/audit_double192_inverse.cjs <validation-root>
```

The audit checks all three accepted cases, original native convolution
captures, first-axis outputs, forward regression and Vulkan logs.
Production runtime, CLI and shaders_out are unchanged in this checkpoint.
Next: combine accepted forward FFT, Double spectral filter multiplication and
inverse FFT on GPU, checking optical/adaptation outputs including crop/clamp.
Remaining visual/quality GPU stages and public Double runtime integration
remain open. This does not establish full GPU HDR-VDP-3 parity.
