# Double transcendental capability and query corpus

Measured 2026-10-30, artifacts:
`validation/double_math_probe_639289555561227814`.

`tests/hdrvdp3_double_gpu/probe_double_transcendentals.ps1` compiles isolated
Double expressions with the installed glslang toolchain. Double `sqrt` and
`fma` compile, pass SPIR-V validation, and retain Double instructions without
`OpFConvert`. Double `log`, `log2`, `exp`, `exp2`, and `pow` are rejected with
"no matching overloaded function found". These are capability measurements,
not GPU numerical comparisons. Successful compilation alone cannot prove parity.

The standalone `double-pow-query-export` CMake target generates 1,158,704
little-endian `(x, exponent, CPU_result)` Double triples (24 bytes per record).
`groups.tsv` records offsets, source fields and lengths. The corpus contains
six per-pixel power calls per band, final error powers and 35 controls covering
signed zero, small/large positive values and infinity. Source captures remain
unchanged and column-major order is preserved.

Inputs come from the existing two distorted pairs and identity across three
displays and five bands. CPU compilation uses MSVC `/fp:strict`. Native exported
parameters determine the exponents. An independent byte/value audit checks
386,208 excitation/psychometric power results against native exports and all
45 final native band errors exactly. Other prepared power results are CPU
oracle values, not independently exported native assertions.

Run:
```powershell
node tests/hdrvdp3_evaluation/audit_double_pow_queries.cjs <probe-root> <native-cpu-root>
```

The existing v9 production path is preserved. Full GPU nonlinear processing
requires explicit Double math implementations or another supported route.
Next: implement an isolated Double logarithm candidate using range reduction
and a measured polynomial, compare every relevant query against CPU/native
results, and retain CPU fallback on any bit mismatch. The corpus is a diagnostic
sample, not a proof for every Double argument, dimension, device or viewing setup.
