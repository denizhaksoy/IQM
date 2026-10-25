# Isolated inverse160 first axis

Checkpoint: `validation/mixed_inverse160_639283665236700141`.
This is an unnormalized first-axis component, not a complete inverse transform
or general-dimension quality acceptance. Production code/guards are unchanged.

`generate_fft160_inverse_axis_shader.ps1` translates the ordinary scalar
FFTW3.3.10 `t2_20` graph and uses the existing ordinary `n1_8` leaf. The source
and GPL provenance are pinned in `fftw_reference/mixed_inverse/SOURCES.json`.
The actual native graph has three64-column groups, each using20 scalar8-point
leaves followed by a20-point twiddle codelet. Backward scalar execution swaps
the real/imaginary pointer roles before/after these forward scalar codelets;
the shader preserves that operation order. Products/sums and helper results
are precise Float32, with separate multiply/add. Twiddle powers1,3,9,19 come
from the codelet descriptor; host double trig is rounded once to Float32,
with exact quadrant controls. No generic FFT library or CPU fallback is used
by the GPU path.

Input is the independently exported official filtered spectrum (`stage2`):
160x192 interleaved column-major complex Float32, text header160 384. Its
preceding MTF preparation used symmetric POST padding, LA replicate POST
padding. Those stages are not executed by this shader. The first inverse
axis transforms160 samples in each of192 columns and writes the transposed
storage, exported/compared as row-major160x192 complex Float32. It applies no
normalization, second axis, padding, filter, mean or DC operation.

Native oracle: `dimension-native-plan-test --inverse160-axis <stage2.txt>
<fresh directory>`, local Octave FFTW DLL, three threads, fresh wisdom,
ESTIMATE|UNALIGNED, out-of-place transform. Its complete whitespace-normalized
plan must be an exact subtree of the previously matched full rank2 candidate.
This confirms the diagnostic graph, not recovery of original cached Octave
plan objects. All stream plans must also agree with each other.

Build `dimension-native-plan-test` from `tests/hdrvdp3_dimensions` and
`fft256-inverse-test` from `tests/hdrvdp3_fft256`. The isolated compiler invokes
glslc, adds explicit SPIR-V DenormPreserve32/RoundingModeRTE32 capabilities and
execution modes, assembles and validates Vulkan1.2. Compile only this shader,
then use the reusable driver:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_fft256/compile_inverse160_axis.ps1 -OutputDirectory <fresh shader directory>
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_dimensions/validate_inverse160_axis.ps1 -NativeDirectory <mixed_fft_plans checkpoint> -PlanExecutable <native exe> -GPUExecutable <GPU exe> -Shader <shader.spv> -OutputDirectory <fresh directory>
```

The driver records source paths/hashes, native plans, GPU exports, per-stream
logs, `results.tsv` and a final `PASS` only after all12 streams pass. Inputs
are asymmetric/impulse/HDR-ramp, ref/test factor1.05, MTF/LA at30ppd; these
are component fixtures, not full-quality pairs. Float32 RTE/denormal-preservation
device capabilities are required. Numerical equality is required; signed-zero
bit identity is not asserted by this checker. No tolerance acceptance.

The first raw glslang/default-control run in `verified/` passed asymmetric ref
MTF but failed asymmetric ref LA with9280 differing values (maximum absolute
error6.018531076210112e-36). Device capability checks alone do not enable
preservation in the shader. That failed run is preserved and excluded from
acceptance. Explicit float-control compilation is under `shader_preserve/`;
the final fresh driver run is `verified_preserve/`. This follows the existing
256-point inverse compiler protocol; no tolerance was relaxed.

Final result: all12 streams pass on Intel Arc Pro Graphics,737280 Float32
values numerically exact (zero maximum error). All12 isolated plans match the
prior full rank2 subtree. `verified_preserve/PASS`, per-stream logs and hashes
are the acceptance records. A forward160x192 asymmetric ref MTF regression
with the newly built checker also passes61440 values; that repeated fixture
is not additional unique input coverage. The accepted SPIR-V declares only
Shader,DenormPreserve,RoundingModeRTE capabilities, with no Float64/Int64.

Next: port the192-point complex inverse second axis using actual backward
SIMD codelets and54/54/52 native batch groups. `t1b.h`, now pinned, selects
BYTWJ1/TWVL1 just like the forward header; do not infer the contract from the
codelet name. Compare unnormalized results before composing the independently
validated fixed30720 division against official `ifft2`. Transposed192-first
geometry,120-sample mean/DC and10x12 residual transforms remain separate work.
