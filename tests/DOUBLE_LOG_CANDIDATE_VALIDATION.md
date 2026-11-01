# Double GPU logarithm diagnostics

Measured 2026-10-31. Artifacts:
`validation/double_log_candidate_639290551273713565`.
Production v9 was not changed or promoted.

## Inputs and execution

`double-log-candidate-test` loads preserved native adaptation and post-pupil
captures from `validation/srgb_csf_639288068563510612`. It uses the existing
two distorted pairs and identity, three display spectra, two image sides.
Each side contributes 9,216 adaptation inputs and 27,648 clamped L/M/rod
post-pupil inputs (the unused S channel is omitted). Clamp endpoints follow
the current JND range, `pow(10,-5)` and `pow(10,5)`. Source column order remains.

There are 663,552 captured inputs and 28 positive normal finite controls.
Both natural and decimal logarithms are evaluated for each input, including
cross-operation diagnostics beyond the corresponding production query type.
Thus 1,327,160 output comparisons do not represent that many unique production
queries. Reused images and identity are overlapping historical coverage.

CPU oracles are MSVC 19.51 `std::log`/`std::log10`, `/fp:strict`; these are not
independently exported native logarithm arrays. Source captures alone do not
establish native-stage or final-score equivalence for a candidate.

Execution used NVIDIA RTX A500 Laptop GPU, Vulkan validation enabled, Double
RTE and signed-zero/infinity preservation. Denormal preservation is unavailable;
subnormal, zero, negative, infinity and NaN arguments are outside this diagnostic
domain. Only Float64 arithmetic appears in inspected shader modules, with no
`OpFConvert`. Modules pass `spirv-val`.

## Measured results

| Candidate | Operation | Inputs | Bit mismatches vs CPU | Maximum ULP |
|---|---|---:|---:|---:|
| `double_log_candidate.comp` | log | 663,580 | 21,893 | 2 |
| Same | log10 | 663,580 | 166,560 | 3 |
| `double_log_compensated_candidate.comp` | log | 663,580 | 99 | 1 |
| Same | log10 | 663,580 | 137 | 1 |

Neither candidate produced nonfinite outputs. Inspected Vulkan logs were clean.
The first candidate uses frexp range reduction, corrected division and an
atanh series through power 41. The second retains high/low Double pairs in
range reduction, series evaluation and conversion to base 10.

The independent Node audit recalculates bit mismatches, ULP distances,
histograms and per-input-group counts from saved arrays and cross-checks the
C++ summaries. Both audits explicitly record `production_accepted:false`.

## Independent binary128 diagnostic

`audit_double_log_quad.cpp`, compiled separately with GCC/libquadmath, checks
all 236 compensated-candidate/CPU disagreements, plus normal controls and a
periodic sample of matching results. It rounds binary128 `logq`/`log10q` outputs
back to Double. It checked 194 log and 232 log10 results:

- All 99 disputed log results matched the GPU candidate; none matched CPU.
- All 137 disputed log10 results matched the GPU candidate; none matched CPU.
- All sampled matching results also matched the binary128 calculation.

This is evidence that the compensated candidate is numerically strong, while
its rounding differs from the current CPU oracle. Binary128 is an additional
finite-precision diagnostic, not a formal proof of correct rounding for all
inputs. Greater accuracy cannot substitute for the requested native parity.
The additional Octave query check below establishes the observed environment's
rounding behavior directly, without claiming a full official metric run.

## Octave query oracle

`export_double_log_queries.m` evaluates the same built-in `log` and `log10`
functions used by the official pipeline on the identical saved input array.
Octave 11.3.0 produced 663,580 query pairs. Every result matched the MSVC CPU
oracle bitwise. The compensated GPU candidate therefore retains 99 log and
137 log10 mismatches against this Octave environment. Input arrays between
candidate runs were independently checked byte-identical.

The Octave process returned exit code 0 with the expected complete output,
but printed `ignoring const execution_exception& while preparing to exit`.
The warning is retained in `octave_queries.log`; output size and every query
were independently audited. This is a query-level check, not a complete metric
execution or a cross-version/native-platform guarantee.

## Reproduction and next step

Compile each shader into a fresh directory using
`compile_double192_multiply.ps1 -ShaderSource <candidate>`; this helper injects
the existing Double execution modes without touching shared shader binaries.
Build `double-log-candidate-test` from `tests/hdrvdp3_double_gpu`; run with
`<native-root> <shader.spv> <fresh-output>`. Save combined process output as
`run.log`. Run:

```powershell
node tests/hdrvdp3_evaluation/audit_double_log_candidate.cjs <candidate-root>
```

The baseline root has `shader`/`run`; the second is under `compensated`.
Both save input, CPU expected and GPU actual arrays and group lineage.
The GCC binary128 tool takes `<run-directory> <fresh-report>`.

Next, investigate a source-defined logarithm implementation whose rounding
matches the measured Octave/CPU behavior; evaluate it on the complete query
corpus and additional boundary inputs. Do not introduce a corpus-specific
correction table or promote either candidate on a tolerance pass. Exact hybrid
v9 remains the production path. The existing compensated candidate can remain
an accuracy diagnostic but is not an accepted parity implementation.
