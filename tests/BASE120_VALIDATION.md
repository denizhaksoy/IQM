# Bounded base120 adaptation validation

Accepted checkpoint: `validation/base120_639283822820535556`, Intel Arc Pro
Graphics. This is a component test, not complete HDR-VDP-3 quality acceptance.

The shader computes the mean of 120 Float32 log samples, then `10^mean`.
It reuses the bounded integer mean implementation from `mean120.comp` and
the existing native-style power helpers. Each input is normal or zero,
within [-5,5], with nonzero exponent at least -90 and exponent spread at most
16. The host checks the domain before dispatch. No Float64 or Int64 shader
capability is used; Float32 DenormPreserve and RoundingModeRTE are explicit.

`generate_base120_reference.m` exports independent Octave single-precision
means and powers. Synthetic coverage comprises 8192 maps, including positive
and negative zero, constant +5/-5, cancellation and seeded random maps.
Another 12 maps use native `log10` of positive standalone official SPYR
residual fixtures from the mean120 checkpoint. Those residual fixtures cover
80x96/96x80, asymmetric/impulse/bounded-HDR patterns and reference/test inputs
at 30 ppd. They are not complete visual-pathway L_adapt exports.

Input layout is 120 row-major uint32 Float32 words per map. Output is two
uint32 words: mean, then power. There is no padding, FFT, filtering or DC
subtraction in this shader. Native generation asserts that each single mean
also equals the once-rounded exact double sum divided by 120.

`gpu/PASS.txt` and `gpu/results.tsv` report zero numerical and bit differences:
16384 synthetic output words plus 24 native-derived output words, **16408
bitwise matches**. This empirical power result applies to these fixtures and
the recorded shader/device contract; it is not a universal pow correctness
proof. No GPU numerical trial failed. Input and output buffers have separate
sizes because the two-word output is smaller than the 120-word input.

Reproduce in fresh directories after building `base120-test` from
`tests/hdrvdp3_dimensions`:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_dimensions/compile_base120.ps1 -OutputDirectory <fresh shader directory>
octave-cli --quiet --no-gui tests/generate_base120_reference.m <fresh native directory> <mean120 native directory>
base120-test <shader directory>/base120.spv <native directory>/manifest.txt <fresh GPU directory>
```

Public MapGPU shape guards remain unchanged. General-shape visual pathway,
SPYR, CSF and map integration, followed by official full-stage and score
comparisons, remain required.

## Actual full-pathway logmaps, 2026-10-24

`validation/general_pathway_639284244350897941` adds two actual native
visual-pathway fixtures: 80x96 and 96x80 asymmetric luminance images with a
localized test distortion, 30 ppd and age24. The new
`generate_general_pathway_reference.m` makes an instrumented copy in the fresh
output directory, retaining the source license and recording source hashes.
It leaves the installed reference intact. Only the copy's function name,
base-directory resolution and trace assignments change. Each fixture runs
both the original and the copy with fresh single wisdom; Q and Q_JOD are
exactly identical. This checks trace instrumentation, not GPU scores.

Exports include both adaptation maps, their mean and log, perceptual P maps,
all normalized SPYR bands, per-band resized logs and CSF, base difference,
base spectrum after official DC zero, selection and scores. Text matrices
are row-major decimal with17 significant digits; `trace.mat` preserves
native types and complex values. Stage classes are recorded in metadata.

`generate_pathway_base120_reference.m` extracts the actual 10x12/12x10
resized logmaps, asserting that native power equals the traced L_mean.
The new checker flag `--pathway` requires exactly two batches/four output
words; the original mode still requires8192 synthetic and12 native-derived
maps. Fresh Release checker and the unchanged accepted SPIR-V pass both
actual maps **bitwise** (`base_gpu/PASS.txt`, four words). The same checker
then passes the original16408-word suite in `base_original_regression/`.
All runs use Intel Arc Pro Graphics. This validates the base mean/power on
real pathway inputs, while upstream GPU production of those inputs remains
unvalidated. Native scores are approximately4.0304899216/4.8697795868 and
3.9588105679/4.7907233238 (Q/Q_JOD), not GPU quality results.

```powershell
octave-cli --quiet --no-gui tests/generate_general_pathway_reference.m <fresh trace directory>
octave-cli --quiet --no-gui tests/generate_pathway_base120_reference.m <fresh native base directory> <trace directory>
base120-test <accepted base120.spv> <native base directory>/manifest.txt <fresh GPU directory> --pathway
```
