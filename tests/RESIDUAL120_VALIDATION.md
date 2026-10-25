# Residual120 FFT, magnitude and GPU pipeline

Standalone checkpoint: `validation/residual120_639283813321637573`.
Pipeline checkpoint: `validation/residual120_pipeline_639283821352963342`.
These are10x12/12x10 component tests, not public general-dimension quality.
No production shader, MapGPU guard or public shape support is changed.

## Native and isolated GPU components

`generate_residual120_reference.m` exports38 matrices per orientation:32
controlled synthetic cases (zeros,constant,impulse,axial/combined sinusoids,
negative zero and seeded varying-scale data) plus6 official bounded SPYR
residuals from the mean120 checkpoint. Inputs are already native-mean-centered
single values, column-major interleaved complex with zero imaginary parts.
Expected raw FFTs are row-major interleaved; native FFT2 uses three threads,
ESTIMATE and freshly cleared single wisdom per matrix. Forward is unnormalized;
there is no padding, filter or extra DC subtraction in the FFT shader.

`dimension-native-plan-test --residual120-plan <source> <fresh output> H W`
probes aligned/UNALIGNED,threads3/1. Both UNALIGNED variants match all9120
Float32 components per orientation exactly. Both aligned candidates differ:
2580 components for10x12,1676 for12x10. No tolerance is accepted. Matching
plans are fresh arithmetic-equivalent candidates, not recovered native cached
plan objects. Selected graphs:

-10x12: r2cf_10 first axis, n1_12 second axis.
-12x10: r2cf_12 first axis, n1_10 second axis.

`generate_residual120_shader.ps1` translates these ordinary scalar FFTW3.3.10
graphs with precise nonfused helpers. New sources preserve GPL notices and
archive/member/LF hashes in `fftw_reference/residual120/`; it reuses pinned
real192 r2cf_12 and complex160 n1_12. `compile_residual120.ps1` adds explicit
DenormPreserve32/RTE32 and validates Vulkan1.2. `gpu/PASS.txt` on Intel Arc Pro
Graphics accepts18240 Float32 FFT components over76 matrices numerically exact.
There are490 sign-bit differences on zero outputs; no whole-FFT bitwise claim.

`generate_residual120_extended_shader.ps1` retains the raw spectrum, computes
each magnitude with the existing integer96-bit `finite_hypot` helper, zeros
only the DC magnitude, then selects the first maximum in native column-major
order. The checker enforces the norm helper's finite normal/zero exponent
domain[-90,100] using the independent expected spectra. Native references use
`abs(fft2(a))`, set magnitude(1,1)=0 and `max(magnitude(:))` with index minus1.
Output per matrix:240 FFT components,120 row-major magnitudes,one zero-based
column-major index stored exactly as Float32 (361 words).

Final `gpu_extended/PASS.txt`:18240 FFT floats numerically exact,9120 magnitudes
and76 indices **bitwise exact**, on Intel Arc Pro Graphics. The same490 zero
sign differences occur only in the raw FFT. Both orientations each have38
matrices. Regenerating extended references leaves raw input/FFT file hashes
unchanged (`native_regression.txt`). No new unique matrices are counted for
the repeat. Initial extended shader compilation failed because the SSBO
declaration replacement missed whitespace; the corrected generator asserts
the declaration and compiles in `shader_extended_verified/`. No numerical
GPU residual test failed.

Build `residual120-test` from `tests/hdrvdp3_dimensions`, compile the isolated
shader with `compile_residual120_extended.ps1`, generate fresh references, run:

```powershell
residual120-test <extended shader.spv> <native directory>/manifest_extended.txt <fresh GPU directory> --extended
```

The original raw mode remains available and is separately accepted in `gpu/`.

## Three-stage GPU composition

`generate_residual120_pipeline_reference.m` independently exports12 bounded
official SPYR inputs (6 per orientation), their native means/centered words,
packed complex inputs and native FFT/magnitude/index outputs. It reuses the
mean120 checkpoint's standalone SPYR component inputs, not complete visual
pathway residuals. Domain guards are checked for mean and norm before dispatch.

`residual120-pipeline-test` dispatches, in one command buffer:

1. mean120.comp: exact mean and single DC subtraction.
2. mean120_pack.comp: uint-only row-major real -> column-major complex packing,
   skipping the emitted mean word and adding positive-zero imaginary parts.
3. The accepted residual FFT/magnitude/index shader.

GPU shader-write/read barriers connect stages. The host uploads the original
inputs, submits all three stages, waits for the fence, then reads intermediate
and final outputs for verification. It does not compute or upload intermediate
means, centered data, packed inputs, spectra or magnitudes during execution.

Result on Intel Arc Pro Graphics (`gpu/PASS.txt`, `gpu/results.tsv`):1452 mean/DC
words,2880 packed words and1452 magnitude/index words match **bitwise**;
2880 raw FFT words match numerically. Observed raw FFT bit differences are
also zero on these12 fixtures, but raw FFT bit identity is not an acceptance
requirement. These12 cases are a subset of the earlier76, not extra unique
source data. The shader controls use no Float64/Int64 capability.

Reproduce after building `residual120-pipeline-test`:

```powershell
glslc tests/mean120_pack.comp --target-env=vulkan1.2 -o <fresh pack.spv>
spirv-val --target-env vulkan1.2 <pack.spv>
octave-cli --quiet --no-gui tests/generate_residual120_pipeline_reference.m <fresh native directory> <mean120 native directory>
residual120-pipeline-test <mean120.spv> <pack.spv> <residual extended.spv> <native directory>/manifest.txt <fresh GPU directory>
```

This is an actual GPU composition pass for the bounded residual component.
Full quality still requires the visual pathway/filter/SPYR/CSF/map integration
and independent full-stage/score comparisons before public guards can change.

## Real visual-pathway residuals, 2026-10-24

Checkpoint `validation/pathway_residual120_639284268283635835` uses the actual
P_reference/P_test maps saved in the full native pathway checkpoint
`validation/general_pathway_639284244350897941`. Native generation reconstructs
official SPYR with sp0Filters/30ppd. Four raw residuals (reference/test for
80x96 and96x80) reproduce the traced centered bands **bitwise** after official
mean subtraction. Raw exponents are9..9, within the bounded mean domain.
No surrogate fixtures or repeated samples are used to satisfy coverage.

`generate_pathway_residual120_reference.m` creates two explicit manifests.
The pipeline checker `--pathway` profile requires two orientations and two
bands per orientation (four matrices), preserving the original12-case mode.
The residual checker `--pathway` profile requires two extended matrices,
preserving the original76-matrix modes. The pipeline manifest contains uint32
words; the standalone residual manifest contains decimal Float32 values,
matching each checker's existing parser contract.

Accepted outputs are `native_float_contract/`,
`gpu_pipeline_float_contract/PASS.txt` and
`gpu_difference_float_contract/PASS.txt`, Intel Arc Pro Graphics:

- Four real raw bands:484 mean/DC words,960 packed words and484 magnitude/index
  words match bitwise;960 FFT components match numerically. Observed FFT bit
  differences are zero, but the FFT acceptance excludes zero-sign bits.
- Two actual test-minus-reference centered bands:480 FFT components match
  numerically;240 magnitudes and2 first-max indices match bitwise. All722
  output words also happen to match bitwise on these fixtures.
- Native assertions connect the two differences to the official trace, with
  exact spectrum identity after official DC zero and exact selected index.
  The difference is **not recentered**. Each reference/test base is centered
  separately before subtraction; only the final magnitude's DC is zeroed.

Input is single, row-major for mean; complex FFT packing is column-major;
output spectrum/magnitude is row-major and index is native column-major.
Forward FFT is unnormalized. No residual padding/filtering is introduced;
upstream maps use the official visual pathway's padding/parameters, age24 and
30ppd. No production shape guard, shader or ABI changes occur.

The initial difference reference accidentally used uint32 text for the
decimal-float parser, causing356/361 comparisons to fail on the first matrix.
This was a fixture encoding error; failed `gpu_difference*` directories and
original `native/` are retained. Corrected references use a fresh directory.
The checker now saves output/results before reporting magnitude mismatch,
so future failures retain diagnostic evidence without relaxing acceptance.

Latest checkers also pass the original12-case pipeline and76-matrix extended
regressions. The latter still has490 raw FFT zero-sign differences only.
Reproduction after a fresh Release build:

```powershell
octave-cli --quiet --no-gui tests/generate_pathway_residual120_reference.m <fresh native> <full pathway checkpoint>
residual120-pipeline-test <mean.spv> <pack.spv> <extended FFT.spv> <native>/pipeline_manifest.txt <fresh pipeline GPU> --pathway
residual120-test <extended FFT.spv> <native>/difference_manifest.txt <fresh difference GPU> --pathway
```

The band-difference test currently uploads the independently generated native
difference. Connecting GPU-centered reference/test outputs through GPU
subtraction to the final FFT is the next integration step; this checkpoint
does not claim that combined dispatch chain or full GPU quality acceptance.

## Combined GPU difference chain, 2026-10-24

The limitation above is resolved in the newer checkpoint
`validation/pathway_difference_gpu_639284272183191896`. New shader
`mean120_difference_pack.comp` reads reference/test mean+centered output pairs,
subtracts test minus reference with one precise Float32 subtraction, and packs
the difference in column-major complex order. It skips both means and does
not subtract another mean. Imaginary input words are positive zero.

The new `--pathway-difference` pipeline profile uploads only four original
raw bands. One command buffer dispatches mean120 (two bands per orientation),
difference/packing (one pair), then the accepted residual FFT/magnitude/index
shader (one difference). Compute barriers join stages; no intermediate host
read, calculation or upload occurs. Golden means, packed differences and
spectra are used only for verification after completion. Native generation
independently asserts agreement with the original full-pathway trace.

`gpu_combined/PASS.txt` and `results.tsv` show zero errors on Intel Arc Pro
Graphics, both10x12 and12x10:484 mean/DC words,480 difference/packing words,
and242 magnitude/index words **bitwise exact**, plus480 FFT components
numerically exact. Observed FFT bit differences are zero, but FFT zero-sign
bits remain outside the acceptance requirement. These are the same two
image pairs used previously; no additional unique source coverage is claimed.

The new shader explicitly uses DenormPreserve32/RoundingModeRTE32 and precise
subtraction (NoContraction decoration). SPIR-V validation passes, with no
Float64/Int64 capabilities. Only this shader was compiled, into a fresh
directory. Native inputs are single, raw residual exponents9..9, mean/120;
FFT is unnormalized with no padding/filter in this component. Upstream native
parameters remain30ppd/age24 and official2H x2W filtering. Production shape
guards and shader binaries are unchanged.

The same fresh Release checker passes both previous four-band `--pathway`
and original12-band modes in `gpu_pathway_regression/` and
`gpu_original_regression/`. No numerical failures occurred in this checkpoint.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_dimensions/compile_mean120_difference_pack.ps1 -OutputDirectory <fresh shader>
octave-cli --quiet --no-gui tests/generate_pathway_residual120_reference.m <fresh native> <full pathway checkpoint>
residual120-pipeline-test <mean120.spv> <difference shader>/mean120_difference_pack.spv <extended FFT.spv> <native>/combined_manifest.txt <fresh GPU> --pathway-difference
```

The upstream SPYR/visual-pathway maps are still native inputs. General-shape
GPU SPYR/filter/resize/CSF integration and full quality scores remain pending.
