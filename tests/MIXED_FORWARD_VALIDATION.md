# 160x192 forward FFT validation

Checkpoint: `validation/mixed192_forward_639283658517194869`.
This extends the isolated real160 first axis with the native 192-point complex
second axis. Public dimension guards and production FFT code remain unchanged.

The pinned FFTW 3.3.10 sources and wrapper/header contracts are in
`fftw_reference/mixed192/SOURCES.json`, with original GPL notices and archive
SHA256. `generate_fft160x192_forward_shader.ps1` composes:

- Real160: the previously validated scalar20x8 first axis.
- Complex192: scalar `n1_4` leaves, then `t1fv_6_avx2` for each24-point inner
  transform, then `t3fv_8_avx2_128` for the outer radix8 stage.
- `t1f.h` explicitly selects **BYTWJ1**, not BYTWJ2. Both SIMD paths round
  sine products separately and fuse cosine products. The selected t3 graph
  is the generated FMA branch; t1 uses the ordinary branch with explicit SIMD
  FMA macros. This is an arithmetic match to the local native candidate,
  not proof of its original build-time preprocessor branch or cached plan.
- Scalar first-axis helpers retain precise results. Vector add/subtract/multiply
  helpers also retain precise results; caller precision alone did not preserve
  rounding across helper boundaries. Host double trig is rounded once to
  Float32 with exact quadrant controls for both24/192 twiddle tables.

Input contract: already padded160x192, Float32 real samples packed as
column-major interleaved complex with zero imaginary components. MTF uses
endpoint-inclusive symmetric POST padding, LA uses replicate POST padding;
both are inherited from prior independently generated native fixtures.
Output is an unnormalized complete complex forward transform, expanded from
the81-frequency Hermitian first-axis half-spectrum. GPU storage is column-major;
the checker exports/compares row-major interleaved values. No filter, inverse,
normalization or DC processing occurs in this unit. Parameters remain30ppd,
ref/test factor1.05 component fixtures, not full-quality pair acceptance.

The reusable driver `hdrvdp3_dimensions/validate_mixed_forward.ps1` takes
`-NativeDirectory`, `-AxisDirectory`, `-Executable`, `-Shader` and a fresh
`-OutputDirectory`. It copies selected MTF/LA inputs/references into fresh
diagnostic fixtures, records their source paths/hashes, checks the complete
forward result against independent official Octave `fft2`, and separately
checks the first-axis compact output against the prior native oracle.
Build `fft256-inverse-test` from `tests/hdrvdp3_fft256`, generate this shader,
compile only it with glslangValidator Vulkan1.2 and run spirv-val.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/generate_fft160x192_forward_shader.ps1 -OutputDirectory <fresh shader directory>
glslangValidator -V --target-env vulkan1.2 <shader directory>/forward160x192.comp -o <shader directory>/forward160x192.spv
spirv-val --target-env vulkan1.2 <shader directory>/forward160x192.spv
```

Accepted results live under `verified/`, with per-stream logs, `results.tsv`,
input/binary hashes and `PASS`. Require numerical equality, not tolerance;
signed-zero bit identity is outside this check. Device Float32 RTE and denormal
preservation capabilities are required. Repeated diagnostic cases are not
additional unique validation inputs.

Result on Intel Arc Pro Graphics: all12 streams (asymmetric/impulse/HDR-ramp,
ref/test,MTF/LA) pass737280 complete-forward Float32 comparisons with zero
mismatches. Their373248 first-axis values remain exact. A fresh generation
after adding header-contract assertions produces identical SPIR-V bytes
(`reproduction_check.txt`); Vulkan1.2 spirv-val passes.

Failed probes are preserved. The initial assembly changed first-axis rounding
(262 first-axis mismatches,32935 complete-forward mismatches). Retaining its
original scalar helpers restored first-axis parity. Precise vector helpers and
the correct t3 branch reduced remaining mismatches; the mistaken BYTWJ2
translation still failed26354 complete-forward values. `t1f.h` established
BYTWJ1 and its corrected translation passed the initial61440-value comparison.
The alternate nonfused complex multiply failed22586 values. None of these
failed probes is an accepted tolerance result.

Native diagnostic `dimension-native-plan-test --complex192-axis <compact input>
<fresh output>` exports the full192 result and eight strided24 intermediates,
with actual plan text. On the asymmetric ref MTF fixture, its full192 output
matches the independently exported official half-spectrum exactly (31104
Float32 values). The three27-column groups select the expected codelets.
These are fresh ESTIMATE|UNALIGNED plans with three threads, not recovered
original Octave cached objects. The production path has no CPU fallback.

The corrected GPU24-point intermediate probe also matches all31104 values
across eight offsets; see `native_second/inner24_gpu_check.txt` and
`INNER24_PASS`. Its raw `gpu_inner24_corrected.log` reports a full-forward
failure intentionally: this export-only shader writes the24-point intermediate
instead of the final192 transform. That raw rejection is not a forward pass;
only the separately compared intermediate values are accepted. The earlier
BYTWJ2 intermediate probe fails and is preserved.

Next: port inverse160 and inverse192 preserving native FMA/order, compose the
already validated fixed30720 division, then verify official inverse outputs.
The transposed192-first shape needs separate work. General quality still needs
120-sample mean/DC,10x12 residual transforms and map-stage integration.
