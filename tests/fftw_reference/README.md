# Isolated scalar FFTW arithmetic tests

This folder contains original FFTW 3.3.10 `n1_8.c`, `n1_16.c`, `t2_8.c`,
`t3bv_16.c`, `simd-avx2-128.h`, `r2cf_128.c`, `n2fv_16.c`, `t3fv_8.c`,
`n1fv_128.c`, its GPL
license (`COPYING`), and a generated GLSL translation for numerical tests only.
Original copyright notices are retained. The shader translation is derived
from these files and is GPL-2.0-or-later. It is not wired into production IQM.

Source: https://www.fftw.org/fftw-3.3.10.tar.gz

Archive SHA256:
`56C932549852CDDCFAFDAB3820B0200C7742675BE92179E59E6215B340E26467`.
Original archive/inspection copies are preserved under `validation/`.

`../generate_fft_codelet_shader.ps1` translates the two straight-line graphs
without algebraic reassociation. Original FFTW-generated C files are unchanged.
Graph 0 is the FMA-oriented graph; graph 1 is its ordinary alternative. Every
temporary is `precise`. For graph 1, product pairs have explicit separately
rounded products. Inverse is obtained by swapping real/imaginary input and
output roles; no 1/N normalization is applied at the leaf-codelet stage.

Runtime rounding probes:

- `fused=0`: separate product/add rounding (selected validated path).
- `fused=1`: explicit FMA on the first product.
- `fused=2`: explicit FMA on the other product in graph-1 product pairs.

The test reports all variants but its exit code now requires **graph=1,
fused=0** to match for all three sizes. Other exact matches in this fixture set do
not establish their equivalence for arbitrary inputs.

## Validated result, 2026-10-16

The CPU oracle calls the actual local Octave single FFTW DLL, with individual
unaligned inverse plans whose printed names are `n1_8` and `n1_16`. It does not
compile this C source to create expected values. Each size has 512 transforms:
constant, impulse, ramp and deterministic complex pseudorandom values at
scales 1e-3, 1, 10 and 1e4 (seed 0x7143a9).

Selected GPU path, Intel Arc Pro Graphics with Vulkan validation enabled:

| Inverse codelet | Transforms | Complex outputs | MaxAbsErr | RMSE | Mismatches |
|---|---:|---:|---:|---:|---:|
| 8 point | 512 | 4096 | 0 | 0 | 0 |
| 16 point | 512 | 8192 | 0 | 0 | 0 |
| 128 point (`n1_16` + `t2_8`) | 512 | 65536 | 0 | 0 | 0 |

The first translation left 257 float-component differences at size 16. Making
both products explicit `precise` intermediates removed those differences.
This demonstrates why product arguments and implicit contraction need care;
turning on FMA everywhere did not reproduce the reference.

The extension to 128 points uses eight decimated 16-point transforms followed
by sixteen in-place twiddle8 groups. Input indices are `j+8*l`; intermediate
indices are `j+8*k`; final output indices are `k+16*j`. The compact FFTW table
stores powers 1,3,7, evaluated with host double sin/cos then rounded once to
float32 and embedded as round-trip literals. No GPU trig is used. Inverse is
implemented by swapping real/imaginary components on entry and exit.

The actual DLL prints `(dft-ct-dit/8 (dftw-direct-8/6 "t2_8")
(dft-direct-16-x8 "n1_16"))` for the 128-point oracle. Thus the composed test
targets the same arithmetic decomposition as the native first inverse axis.
All three sizes together cover 77824 complex outputs / 155648 float components.
Strict exit success requires graph=1/fused=0 at all three sizes. Other modes
also change the leaf FMA policy and remain diagnostic comparisons only.

No Vulkan validation errors were found. These are isolated unnormalized
inverse primitives and a 1D 128-point transform, not a 128x128 inverse FFT or
a full HDR-VDP parity result. Subsequent second-axis and complete 2D inverse
validation are recorded below.

128-point evidence: `validation/fft_inverse128_initial_comparison.log` (exit 0),
`validation/fft_inverse128_reference_639277431765714130/codelet_plans.txt`,
`validation/fft_inverse128_initial.spv` and
`validation/fft_inverse128_build_reviewed.log`. Original sources now also
include unmodified `t2_8.c`, under the same preserved license and attribution.

## Reproduce

### Second inverse axis (validated separately)

The generator also translates the ordinary `t3bv_16` graph to per-complex
`vec2` arithmetic. This graph retains explicit AVX2 FMA rounding inside
`VZMUL`/`VZMULJ` and its four final constant FMA operations. It is different
from choosing the generic FMA-oriented generated butterfly graph (which failed
comparison). Twiddle powers are 1,3,9,15. Sixteen decimated `n1_8` transforms
read `j+16*l`; eight twiddle16 groups write `k+8*j`. No normalization is used.

The CPU oracle uses **in-place**, stride-128 complex transforms. Out-of-place
planning selected scalar `t2_8`, so that earlier run is not second-axis parity.
The final oracle rejects plans missing `t3bv_16_avx2_128` or `n1_8`.
GPU SSBO fixtures are serialized as contiguous independent transforms; this
test validates arithmetic and within-transform layout, not full 2D dispatch.

Selected graph=2/fused=0 results:

| Native group size | Transforms | Complex outputs | MaxAbsErr | RMSE | Mismatches |
|---|---:|---:|---:|---:|---:|
| 128 | 512 | 65536 | 0 | 0 | 0 |
| 6 | 516 | 66048 | 0 | 0 | 0 |
| 2 | 512 | 65536 | 0 | 0 | 0 |

The 6/2 groups reproduce the main/tail batch sizes in the native 22-thread
inverse plan. This is 197120 complex outputs across three group sizes; full
128x128 inverse composition, cross-axis layout and normalization were validated
in the subsequent test below.

Evidence: `validation/fft_second_axis_ordinary_comparison.log`,
`validation/fft_second_axis_group6_comparison.log`,
`validation/fft_second_axis_group2_comparison.log` and their printed plans in
`fft_second_axis_inplace_639277442984315744`,
`fft_second_axis_group6_639277444124781670`,
`fft_second_axis_group2_639277444365317727` under validation/.
Shader: `validation/fft_second_axis_ordinary.spv`. Failed initial FMA-oriented
and out-of-place experiments are preserved, not counted as passes.

```powershell
# After generating/compiling the shader and building both tools:
& './build-vs2026/bin/Release/IQM-fftw-plan-test.exe' --second-axis '<new reference directory>' 6 # 128 or 2 also supported
& './build-vs2026/bin/Release/IQM-fft-codelet-test.exe' '<reference directory>' '<shader.spv>' --second-axis
```

### Leaf and first-axis regression

```powershell
cmake --build build-vs2026 --config Release --target IQM-fftw-plan-test IQM-fft-codelet-test
# Windows may require a reviewed, process-only execution-policy exception:
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tests/generate_fft_codelet_shader.ps1
glslc tests/fftw_reference/fft_codelet_test.comp -o '<new build directory>/fft_codelet_test.spv' --target-env=vulkan1.2
& './build-vs2026/bin/Release/IQM-fftw-plan-test.exe' --codelets '<new reference directory>'
& './build-vs2026/bin/Release/IQM-fft-codelet-test.exe' '<reference directory>' '<build directory>/fft_codelet_test.spv'
```

Do not change machine-wide script policies or run the destructive blanket
shader compiler script. The test uses host-coherent storage buffers with
host/compute barriers and waits for completion before reading all values.

## Complete normalized 128x128 inverse (2026-10-16)

`--inverse2d` composes the proven axis routines in two GPU dispatches. Buffers
contain interleaved complex float32 matrices in column-major order. Graph 3
processes each contiguous column with the scalar 128-point routine and writes
the intermediate transposed. A compute write/read barrier precedes graph 4,
which processes contiguous rows with the SIMD arithmetic, transposes back,
and divides once by 16384. Descriptor sets use distinct input/intermediate/
output storage buffers; completion is fenced before host comparison.
There is no DC removal, cropping, or real-only comparison: both components of
every normalized complex output are compared with strict zero tolerance.

CPU fixtures come from actual local Octave single FFTW, 22 threads,
estimate/unaligned out-of-place 2D planning. The saved plan is checked for
the expected scalar first axis and SIMD second axis. Independently,
`validate_inverse2d_reference.m` checks every expected matrix against native
Octave `ifft2` and the single-plan probe, asserting the actual thread count.

| Fixtures | Matrices | Complex outputs | MaxAbsErr | RMSE | Mismatches |
|---|---:|---:|---:|---:|---:|
| Constant, off-center impulse, asymmetric ramp, random at four scales | 7 | 114688 | 0 | 0 | 0 |
| Asymmetric ref/test MTF + adaptation stage2 | 4 | 65536 | 0 | 0 | 0 |
| Diagonal ref/test MTF + adaptation stage2 | 4 | 65536 | 0 | 0 | 0 |
| Impulse ref/test MTF + adaptation stage2 | 4 | 65536 | 0 | 0 | 0 |

Total: **19 matrices / 311296 complex outputs / 622592 float components**.
No Vulkan validation errors found. Leaf/first-axis regressions also pass with
the extended shader. Existing preprocessing production path remains unchanged;
this establishes test-only inverse FFT parity on these fixtures, not forward
FFT, log/exp, or full HDR-VDP metric parity.

Evidence under validation/:

- `fft_inverse2d_initial_comparison.log`, `fft_inverse2d_{asymmetric,diagonal,impulse}_comparison.log`.
- `fft_inverse2d_reference_639277446909181448` (synthetic).
- `fft_inverse2d_asymmetric_639277447266540857`.
- `fft_inverse2d_diagonal_639277447313943663`.
- `fft_inverse2d_impulse_639277447340166724`.
- Each reference directory contains its exact 2D plan, source metadata,
  round-trip input/expected values and `octave_reference_check.log` (PASS).
- `fft_inverse2d_initial.spv`, `fft_inverse2d_build.log`,
  `fft_inverse2d_leaf_regression.log`.

The snapshot fixture sources are the existing `spyr_spatial_17921430489879733`,
`spyr_spatial_17921430997117838`, `spyr_spatial_17921431152989965` directories.
Input axis metadata is honored when serializing each fixture to column-major.
These are same-input inverse comparisons, not claims that their original GPU
stage2 spectra already match the complete official preprocessing chain.

## Forward first-axis R2C128 (2026-10-16)

Graph5 translates the ordinary generated `r2cf_128` graph, with separate
product/add rounding and `precise` intermediates. Original R0/R1 point at
even/odd real samples: R0[k] becomes samples[2*k], R1[k] samples[2*k+1].
The graph outputs 65 bins, with imaginary DC/Nyquist initialized to zero;
the test reconstructs bins65..127 by Hermitian conjugation. No normalization
or DC removal. Input SSBO retains interleaved complex format with zero imaginary
components. This is the first 1D axis, not a complete forward 2D transform.

CPU `--r2c` oracle uses actual single FFTW, one-thread estimate/unaligned real
128-point planning and rejects a plan missing `r2cf_128`. An independent Octave
check computes single `fft(real(input),[],1)` on every fixture vector. All four
sets pass both GPU zero-tolerance and native Octave comparisons:

| Set | Transforms | Full-spectrum complex values | MaxAbsErr | RMSE | Mismatches |
|---|---:|---:|---:|---:|---:|
| Constant, impulse, ramp, seeded random at four scales | 512 | 65536 | 0 | 0 | 0 |
| Asymmetric ref/test MTF/adaptation stage0 columns | 512 | 65536 | 0 | 0 | 0 |
| Diagonal ref/test MTF/adaptation stage0 columns | 512 | 65536 | 0 | 0 | 0 |
| Impulse ref/test MTF/adaptation stage0 columns | 512 | 65536 | 0 | 0 | 0 |

Total 2048 transforms: 133120 independently computed half-spectrum bins,
262144 full-spectrum complex values including conjugate reconstruction.
No Vulkan validation errors found. Complete synthetic inverse2d regression
also passes with the extended shader. Production code remains unchanged.

Artifacts under validation/: `fft_r2c_initial.spv`, `fft_r2c_build.log`,
`fft_r2c_final_build.log`, `fft_r2c_initial_comparison.log`,
`fft_r2c_{asymmetric,diagonal,impulse}_comparison.log`,
`fft_r2c_inverse2d_regression.log`. Reference directories (each includes plan,
source metadata, input/expected round-trip text, and Octave PASS log):

- `fft_r2c_reference_639277449506445875`
- `fft_r2c_asymmetric_639277450118078253`
- `fft_r2c_diagonal_639277450256072226`
- `fft_r2c_impulse_639277450390040066`

Original r2cf_128.c SHA256:
`9333D9DC8E068ABE1CA1C6F48558C42ADAA2B1B86EB0E60E0A220C249C65192F`.
The initial synthetic GPU log labels graph5 as inverse; this was a reporting
label only, corrected to forward-r2c in the subsequent built harness.

```powershell
& './build-vs2026/bin/Release/IQM-fftw-plan-test.exe' --r2c '<new reference directory>' # optionally append GPU export directory
& './build-vs2026/bin/Release/IQM-fft-codelet-test.exe' '<reference directory>' '<shader.spv>' --r2c
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet --no-gui tests/validate_r2c_reference.m '<reference directory>'
```

## Forward second-axis SIMD and distinct tail (2026-10-16)

Graph6 composes ordinary `n2fv_16` and `t3fv_8` SIMD graphs: eight decimated
16-point transforms read j+8*l, then sixteen twiddle8 groups write k+16*j.
Twiddle powers are 1,3,7; AVX2 complex multiplies retain explicit FMA.
The ordinary n2fv_16 graph also has four explicit constant FMAs. `STM2` stores
map float offset2*k to complex indexk; `STN2` is a no-op in AVX2-128.

**Correction/clarification of native tail selection:** the 22-thread forward
plan processes the 65 compact frequencies in 21 groups of3, then one group of2.
The last group uses `n1fv_128_avx2_128`, not n2fv_16/t3fv_8. Graph7 separately
translates this ordinary 128-point SIMD graph. The oracle rejects unexpected
plans, so the first attempted group2 composite fixture was correctly rejected
before numerical comparison. Do not label that rejected run a GPU failure.

CPU `--forward-second <new directory> [65|3|2]` plans actual single FFTW
in-place, one-thread estimate/unaligned, complex stride=group and distance1.
It asserts n2fv_16/t3fv_8 for groups65/3, and n1fv_128 for group2. GPU fixtures
serialize each transform contiguously; full 2D compact layout remains untested.

| Group | Selected GPU graph | Transforms | Complex values | MaxAbsErr | RMSE | Mismatches |
|---|---:|---:|---:|---:|---:|---:|
| 65 | 6 | 520 | 66560 | 0 | 0 | 0 |
| 3 | 6 | 513 | 65664 | 0 | 0 | 0 |
| 2 | 7 | 512 | 65536 | 0 | 0 | 0 |

Total **1545 transforms / 197760 complex outputs**. Constant/complex impulse/
ramp and seeded random values at four scales; zero-tolerance full complex
comparison. No Vulkan validation errors found. R2C and complete synthetic
inverse2d regressions pass with the extended shader. This is a same-input DLL
oracle comparison; full native Octave fft2 comparison is the next step.

Evidence under validation/: `fft_forward_second65_comparison.log`,
`fft_forward_second3_comparison.log`, `fft_forward_second3_final_comparison.log`,
`fft_forward_tail2_comparison.log`, `fft_forward_simd_r2c_regression.log`,
`fft_forward_simd_inverse_regression.log`. Reference dirs:

- `fft_forward_second65_639277453522684322`
- `fft_forward_second3_639277453764368273`
- `fft_forward_tail2_639277454446772600`

Each includes exact printed plan and input/expected values. Shader artifacts:
`fft_forward_second_initial.spv` (groups65/3), `fft_forward_tail_initial.spv`
(adds graph7, final group3/tail/regressions). Build logs:
`fft_forward_second_build.log`, `fft_forward_tail_build.log` (both exit0).
Rejected initial group2: `fft_forward_second2_639277453829417292`.

```powershell
& './build-vs2026/bin/Release/IQM-fftw-plan-test.exe' --forward-second '<new group3 directory>' 3
& './build-vs2026/bin/Release/IQM-fft-codelet-test.exe' '<group3 directory>' '<shader.spv>' --forward-second
& './build-vs2026/bin/Release/IQM-fftw-plan-test.exe' --forward-second '<new group2 directory>' 2
& './build-vs2026/bin/Release/IQM-fft-codelet-test.exe' '<group2 directory>' '<shader.spv>' --forward-tail
```

## Complete forward128x128 (2026-10-17)

Graph8 applies real128 to each input column and stores65 frequency vectors
transposed in an intermediate SSBO (65*128 values within each16384-value matrix
allocation). A compute write/read barrier precedes graph9: frequencies0..62 use
the composite forward16/twiddle8 graph, while63..64 use forwardTail128. The
result is stored column-major. For0<k<64 the same invocation writes the partner
`F((-x) mod128,128-k)=conj(F(x,k))`; both frequency axes are reflected. DC and
Nyquist planes are written directly. No normalization or DC removal is applied.
All output locations are owned by exactly one invocation; a fence precedes
host readback. Real modes reject nonzero imaginary input instead of dropping it.

CPU `--forward2d` uses actual single FFTW,22-thread estimate/unaligned R2C2D,
including Octave's compact output offset8064 inside a full16384-complex buffer.
It asserts the presence of real128, forward main SIMD and distinct tail graphs.
`validate_forward2d_reference.m` independently compares every expected matrix
against native single `fft2` and the native probe (actual thread count22).

| Set | Matrices | Full complex values | MaxAbsErr | RMSE | Mismatches |
|---|---:|---:|---:|---:|---:|
| Constant, off-center impulse, asymmetric ramp, random four scales | 7 | 114688 | 0 | 0 | 0 |
| Asymmetric ref/test MTF/adaptation stage0 | 4 | 65536 | 0 | 0 | 0 |
| Diagonal ref/test MTF/adaptation stage0 | 4 | 65536 | 0 | 0 | 0 |
| Impulse ref/test MTF/adaptation stage0 | 4 | 65536 | 0 | 0 | 0 |

Total19 matrices /311296 full complex values /622592 float components,
including Hermitian reconstruction;158080 compact bins computed independently.
Both components compared at zero tolerance. No Vulkan validation errors found.
Full synthetic inverse2d, R2C, composite forward main and distinct tail
regressions also pass with final rebuilt tools. Production code is unchanged.

Evidence under validation/:

- `fft_forward2d_build.log`, `fft_forward2d_final_build.log` (completed exit0).
- Shader `fft_forward2d_initial.spv`.
- GPU logs `fft_forward2d_initial_comparison.log`,
  `fft_forward2d_{asymmetric,diagonal,impulse}_comparison.log`.
- Oracle directories `fft_forward2d_reference_639278561661925639`,
  `fft_forward2d_asymmetric_639278562033305292`,
  `fft_forward2d_diagonal_639278562208629720`,
  `fft_forward2d_impulse_639278562273635020`.
- Each contains forward2d_plan.txt, input128.txt/forward128.txt, source.txt,
  and octave_reference_check.log with explicit PASS and zero mismatches.
- `fft_forward2d_final_639278562885963009` contains the final fresh forward/
  inverse oracle fixtures and five passing GPU regression logs.

```powershell
& './build-fft-parity/Release/IQM-fftw-plan-test.exe' --forward2d '<new directory>' # optional final GPU export directory
& './build-fft-parity/Release/IQM-fft-codelet-test.exe' '<directory>' '<shader.spv>' --forward2d
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet --no-gui tests/validate_forward2d_reference.m '<directory>' '<native probe directory>'
```

Next integrate proven transforms into an explicit experimental preprocessing
path, preserving default settings and layout traces, then compare every stage
against official HDR-VDP. Same-input FFT equality does not prove filter
construction, log/exp/subnormal behavior, adaptation or whole-metric parity.

```powershell
# Build/generate/compile as above, then use new output directories:
& './build-vs2026/bin/Release/IQM-fftw-plan-test.exe' --inverse2d '<new reference directory>' # synthetic
# Optional last argument imports four ref/test MTF/adaptation stage2 snapshots:
& './build-vs2026/bin/Release/IQM-fftw-plan-test.exe' --inverse2d '<another new directory>' '<GPU export directory>'
& './build-vs2026/bin/Release/IQM-fft-codelet-test.exe' '<reference directory>' '<shader.spv>' --inverse2d
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet --no-gui tests/validate_inverse2d_reference.m '<reference directory>' '<native probe directory>'
```
