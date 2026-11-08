# Single GPU FFT128 against the current reference - 2026-11-07

This isolated experimental path uses the existing FFTW-derived test shader and
preserves its GPL-2.0-or-later attribution/source/COPYING. It is not wired into
the production quality backend and does not establish a4K GPU FFT.

A new host interface dispatches the2axes with GPU barriers and returns complete
column-major interleaved Float32 spectra. Forward inputs are real; inverse
normalization divides once by16384, with no DC modification. Oracle:actual
installed Octave FFTW DLL,3threads,estimate/unaligned flags82, fresh plans.

Initial run01 FAILED107,861bit comparisons in forward transforms while all
inverse transforms matched. Historical shader assumes22thread grouping. The
actual3thread plan buffers5frequency vectors, distributes2+2+1:fourdirect
n1fv_128 transforms thenonecomposite n2fv_16/t3fv_8 transform. Mismatch positions
confirmed this pattern. Originalshader andfailedoutput tensors retained.

A separate derived shader changesonlythat graph selection, keeping arithmetic
unchanged. Corrected run02 PASSED16forward/inverse cases,524,288Float32 components,
zero bit mismatches including signedzero. Inputs:constant,impulse,pattern and
seeded random values atmultiple scales. GPU:NVIDIA RTX A500, Vulkan validation
on; no reportederrors. Shader compilation/spirv-val passed.

Artifacts:validation/fft128_acceptance_20261107_run01(failed),
validation/fft128_acceptance_20261107_run02(passed),
validation/fft128_shader_20261107_run02(shader,derivation,sourcehashes,probelog).
Generator:tests/generate_fft128_threads3_shader.cjs. Fixed128only; other
sizes/threadcounts/plans unsupported. No CPU numerical fallback.

Separate GPU filter multiplication passed5cases/232,972Float32 values, including
finite signedbit patterns,subnormals,signedzero andcoefficient rounding ties.
GPU reproduces Double-to-Single coefficient rounding thenSingle multiplication,
using Float64/integer rounding to avoid flushing subnormals. Coefficients[-1,1].
Artifact validation/fft_filter_acceptance_20261107_run01; source/shader hashes in
validation/fft_filter_shader_20261107_run01. Coefficients are supplied byCPU in
these tests; GPU filter-model generation remainsunfinished.

Combined forward/filter/inverse/GPUcrop chain PASSED4cases/28,671croppedFloat32
values withzero bit mismatches versusactualCPUFFTW convolve. Casescoveridentity,
zerofilter,twoGaussianfilters andfull/square/oddrectangular crops. Artifact:
validation/fft128_convolution_acceptance_20261107_run01 (sources/shader/exehashes).
The filter-model coefficients in these tests are CPU-generated fixtures.

The fixed128provider for FFTBackendSingle also PASSED the4convolution cases
in a fresh run; unsupported64x64 shapes were correctly rejected before dispatch.
Evidence:validation/fft128_provider_acceptance_20261107_run01.
Unsupported sizes fail rather than falling back toCPU. This provider
still uses synchronous host transfers between GPU stages; it is not aresident
pipeline or an integratedfullquality backend. Main LIVE archive jobs useunchanged
frozenhybridruntime.
