# Isolated real160 forward axis

Checkpoint: `validation/mixed160_axis_639283655659964334`.
This is a diagnostic component, not public general-dimension quality support.

`generate_fft160_forward_axis_shader.ps1` translates the ordinary scalar
FFTW 3.3.10 `r2cf_20`, `r2cfII_20`, `hc2cf_20` and existing `r2cf_8`
graphs. New source files retain their original GPL notices; `mixed160/SOURCES.json`
pins their LF SHA256 values and the verified archive hash. The codelet graph
uses separate Float32 multiply/add operations, precise intermediates and
host double trigonometry rounded once to Float32 twiddle constants.

`dimension-native-plan-test --forward160-axis <stage0.txt> <fresh directory>`
exports an isolated native half-spectrum. It uses the local Octave FFTW DLL,
three threads, fresh wisdom, ESTIMATE and UNALIGNED. The observed plan has
three groups of 64 columns, each with the same 20-by-8 scalar graph as the
matching rank2 candidate. It is not a recovered original Octave cached plan.

Input is already padded: 160 real samples per column, 192 columns, column-major
interleaved Float32 with zero imaginary components. The input text header is
160 384. Output is 192 consecutive columns of 81 complex frequencies, header
192 162. The transform is unnormalized; this unit performs no padding, filter,
second axis, inverse, normalization or DC processing.

GPU invocation (the fixture label is `ref` even for a test-derived fixture;
the native diagnostic copies its selected input to `ref_mtf_stage0.txt`):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/generate_fft160_forward_axis_shader.ps1 -OutputDirectory <fresh shader directory>
glslangValidator -V --target-env vulkan1.2 <shader directory>/forward160_axis.comp -o <shader directory>/forward160_axis.spv
spirv-val --target-env vulkan1.2 <shader directory>/forward160_axis.spv
# Build fft256-inverse-test from tests/hdrvdp3_fft256 and
# dimension-native-plan-test from tests/hdrvdp3_dimensions.
dimension-native-plan-test --forward160-axis <stage0.txt> <fresh native directory>
fft256-inverse-test <native directory> <shader.spv> ref <fresh GPU directory> --forward160-axis
```

Acceptance records are `results.tsv`, per-stream logs and `PASS` under the
checkpoint. Numerical equality is required; signed-zero bit identity is not
asserted by this FFT checker. Float32 RTE and denormal-preservation device
capabilities are required. No tolerance acceptance is used.

Result: all 12 streams (asymmetric/impulse/HDR-ramp, ref/test, MTF/LA) pass
on Intel Arc Pro Graphics: 373,248 Float32 values, zero mismatches and zero
maximum absolute error. Every stream uses 192 transforms of length160.
`plan_structure_check.txt` confirms the whitespace-normalized isolated plan
is an exact subtree of the prior matched rank2 plan; all 12 plans agree.
This establishes fixture parity with the native candidate first-axis graph,
not independent full-forward or full-quality parity.

Next: port the 192-point complex second axis, verify the complete forward
transform against independently exported official `fft2`, then port inverse
graphs. The transposed 192-first-axis case needs its own graph. Full quality
also still requires 120-sample DC handling and 10x12 residual transforms.
