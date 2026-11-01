# Source-order Double GPU logarithm parity

Validated 2026-10-31. Artifacts:
`validation/double_log_winlibm_639290554223038982`.

Both isolated GPU candidates produced **zero bit differences** for 672,036
positive normal finite arguments, for both `log` and `log10`: **1,344,072 unique
Double output words**, matching the observed MSVC and Octave query oracles.
This supersedes the earlier candidates' 99/137 query mismatches, but does not
integrate logarithms into the production metric yet.

## Implementation and provenance

The generator `generate_winlibm_log_candidate.cjs` translates the normal-input
paths of AMD's MIT-licensed [win-libm](https://github.com/amd/win-libm/tree/6121d02aef0c775947d5dc655d7fc4ccfe2144ab)
`log.asm` and `log10.asm`. The pinned upstream commit is
`6121d02aef0c775947d5dc655d7fc4ccfe2144ab`. Original assembly sources and license
notices are preserved in `upstream`; per-file SHA256 is in `upstream_hashes.json`.
Generated shaders retain the upstream MIT notice.

Five 257-entry tables retain their original bit patterns using `uvec2` and
`packDouble2x32`, rather than decimal regeneration. The generator verifies
table lengths and permits only the explicit final zero padding word. Normal
mantissa/exponent reduction, table indexing, the `abs(x-1)<.0625` branch and
separate log10 near-one reconstruction follow the source operation order.
Two variants preserve explicit FMA or separate multiplication/addition paths.
GPU near-one division uses the already measured FMA residual correction.
No input-specific lookup or rounding correction table is introduced.

Source-family agreement does not prove which exact implementation this Windows
runtime executes. Both variants happened to agree on this entire test corpus;
they are not established equivalent for every Double argument.

## Coverage

The existing native-derived corpus contains 663,552 inputs from two distorted
pairs and identity across three displays and two image sides, plus 28 controls.
It preserves column layout and JND endpoint clamping. Both operations are
evaluated for every input, including cross-operation diagnostics beyond the
corresponding production query type.

The new `--boundaries` option adds 8,456 controls: representable neighbors of
all normal powers of two, 257 table centers at three exponents, and both near-one
branch thresholds. Subnormal or infinite neighbors are excluded. Total unique
coverage is 672,036 input words. The smaller runs and two variants overlap;
their output counts must not be added to inflate coverage.

Each of `fma`, `sse`, `fma_boundaries`, `sse_boundaries` has saved input, CPU
expected and GPU actual arrays. Independent Node audits recompute counts,
bit differences and ULP distances. The final audit confirms byte-identical
inputs and exact CPU/GPU/Octave outputs. Octave 11.3.0 evaluated the same
built-in query functions used by the official pipeline. This is a query test,
not a new complete official metric execution or score comparison.

Vulkan validation logs contain no errors. Modules pass SPIR-V validation and
retain Double RTE/signed-zero modes without Float32 types or conversions.
Device: NVIDIA RTX A500 Laptop GPU; denormal preservation unavailable. Zero,
negative, subnormal, NaN and infinite arguments remain outside the candidate
domain. No other-device, other-platform or all-input parity guarantee follows.

Octave printed its previously observed shutdown `execution_exception` warning;
complete output size and every result were independently verified. The combined
PowerShell boundary invocation reported exit 1 because of native stderr handling;
no explicit Octave failure guard fired. The saved warning and successful final
array audit are retained rather than representing that invocation as clean.

## Reproduce

Generate each shader into a fresh path:

```powershell
node tests/hdrvdp3_evaluation/generate_winlibm_log_candidate.cjs <upstream> <fresh.comp> fma
```

Use `sse` for the second variant. Compile with the existing
`compile_double192_multiply.ps1 -ShaderSource <fresh.comp>` into a fresh directory.
Build `double-log-candidate-test`, then run
`<native-root> <shader.spv> <fresh-run> --boundaries`, capturing `run.log`.
Generate the Octave oracle with `export_double_log_queries.m` and run:

```powershell
node tests/hdrvdp3_evaluation/audit_winlibm_log.cjs <root> <previous-log-root>
```

Next: add an optional logarithm backend stage/runtime version, then replace
selected CPU log/query preparation in the JND path. Repeat full intermediate
and score comparisons and legacy runtime checks before enabling it. The
current v9 remains unchanged. Nonlinear `exp` and `pow` still require separate
work; exact isolated logarithms do not complete the metric.
