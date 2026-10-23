# Rectangular SPYR and FFT components

The2026-10-22 checkpoint is
`validation/spyr_dimensions_639282601672947206/`. These are isolated
same-input components; public quality API dimensions are unchanged.

The actual production SPYR coefficients and Vulkan dispatch are exercised
through a CMake-gated test entrypoint in evaluation.cpp. Sixteen cases cover
64x128,128x64,63x65,65x63,80x96,96x80,127x129,129x127 (height x width), each at
30/60ppd. All339,328 exported band values equal official v3.0.7 native single
get_band results numerically. GPU raw bands are divided by2^band to match
get_band's contract. This is a synthetic10+cos(x/17)+sin(y/23) input domain.
The initial gpu_* exports compared raw bands with normalized native bands and
failed; they are preserved and excluded. Accepted exports use normalized_*.

Separate source-generated FFTW shaders implement:

| Component | Shapes (height x width) | Compared Float32 components |
| --- | --- | ---: |
| Real forward FFT | 128x256,256x128 | 131072 |
| Complex inverse FFT | 128x256,256x128 | 131072 |
| Real residual FFT | 8x16,16x8 | 512 |

Each shape uses one deterministic native random fixture. All262,656 GPU
components compare exactly, without tolerance; no signed-zero bit identity
claim. FFT inputs are already FFT-sized: no padding/filtering is tested.
Inputs and outputs are column-major interleaved in shaders; the checker
transposes row-major fixture text on upload and export. Inverse normalization
is once1/32768 on the second axis. Forward/residual are unnormalized; no DC
removal is performed. Shader contracts are128256 inverse,256128 forward and83
residual, with Float32 DenormPreserve/RTE and no Float64/Int64 arithmetic.

The local native probe is checked against ordinary fft2/ifft2 on exactly the
same input before accepting each plan. Native single threads=3, estimate
planner. The256x128 forward second axis uses n1fv_128 for40 vectors and
t3fv_8+n2fv_16 for3 tails in each43-vector thread partition;128x256 uses
t3fv_16+n2fv_16. Inverse first axis uses t2_8+n1_16 or t2_16+n1_16 according
to length; second axis uses t3bv_32+n1_8 or t3bv_16+n1_8. Original FFTW source
notices and COPYING are retained by component generators. Generated artifacts
carry GPL-2.0-or-later provenance from the existing source-backed generators.

Build SPYR validation with tests/hdrvdp3_evaluation and
`-DIQM_HDRVDP3_DIMENSION_DIAGNOSTIC=ON`, target IQM-spyr-dimension-test.
Generate native fixtures with generate_dimension_frontier.m and compare each
case using `IQM-spyr-dimension-test native-case ppd band-count fresh-output`.
The diagnostic entrypoint is absent from default builds. The standalone
evaluation CMake target now enables /EHsc to preserve exception unwinding.

Generate FFT fixtures with generate_rectangular_fft_reference.m, providing
the directory of the existing inspect_fftw_native.oct probe. Compile shaders
using tests/hdrvdp3_fft256/compile_rectangular_inverse.ps1 with no switch,
`-Forward` or `-Base`, always in fresh directories. Build fft256-inverse-test
from tests/hdrvdp3_fft256. Call it with native-case, shader.spv, ref,
fresh-output and --rectangular-inverse/--rectangular-forward/--rectangular-base.

Remaining integration:rectangular MTF/LA prepare/filter/finish, MapGPU128-sample
DC/adaptation and rectangular resize/masking, residual first-max/frequency,
then continuous native stage and official quality comparisons. General odd
FFT sizes and non-dyadic double comparison-mask coefficients need separate
contracts. Component passes do not authorize widening public guards.
