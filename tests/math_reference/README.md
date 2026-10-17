# Source-backed single-precision math experiment

> Publication scope: this document describes locally validated work. The implementation, new test tools and full artifacts are not included in this documentation checkpoint. See SESSION_HANDOFF.md at the repository root for current status; earlier measurements below retain their historical scope.


This replaces the earlier local UCRT binary-table inspection with fixed,
licensed source inputs. Original MIT-licensed AMD win-libm assembly files and
their notices are preserved in amd_win_libm/. See LICENSE.txt in that folder.
Source: https://github.com/amd/win-libm/tree/6121d02aef0c775947d5dc655d7fc4ccfe2144ab.
SOURCES.json records exact commit and LF-normalized SHA256 hashes; the generator
verifies them before reading constants/tables. No Windows DLL is loaded/read.

native_math.glsl adapts the source's FMA log/exp graphs to GLSL float32.
Exp uses compensated float pairs instead of double arithmetic. Split products
avoid the failed FMA-residual experiment. The near-one log division is rounded
with integer long division, because a fresh seed exposed a GPU division last-bit
difference and finite compensated division lost a midpoint at1-eps(single).

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tests/generate_reference_math_shaders.ps1 -OutputDirectory '<new directory>'
# Compile each generated source explicitly; do not run blanket shader cleanup.
glslc '<directory>/math.comp' -o '<directory>/math.spv' --target-env=vulkan1.2
glslc '<directory>/prepare.comp' -o '<directory>/prepare.spv' --target-env=vulkan1.2
glslc '<directory>/finish.comp' -o '<directory>/finish.spv' --target-env=vulkan1.2
glslc '<directory>/multiply.comp' -o '<directory>/multiply.spv' --target-env=vulkan1.2
# Use the documented IQM_FFT_MATH_SHADER_DIR experiment for integration.
```

Domain validated here: positive normal log inputs[1e-6,1e10], exp inputs[-80,80].
This is not a general replacement for libm: exceptional values, subnormal log
inputs and exp underflow/overflow are outside this implementation's scope.
Removing the Windows binary dependency does not prove identical results on
every Vulkan driver or against other platforms' Octave/libm builds.

Isolated final source-backed shader passes all3 datasets:65536 pairs each,
196608 tested pairs /393216 function evaluations, zero differences against
native single Octave. Datasets include repeated image-derived inputs plus
distinct random seeds; do not call them196608 unique random samples.
Initial expanded-seed log failure and unsuccessful compensated-division fix
are retained under validation/, not counted as passes.

Evidence under validation/: source_math_comparison.log (initial source-backed
baseline), source_math_seed20261017_comparison.log (initial one-value failure),
source_math_division_comparison.log (failed midpoint correction),
source_math_exact_division_639278581939061846/check_*.log (final baseline and seed20261017),
source_math_seed121339_final.log. Final integration shaders originate in that
same source_math_exact_division directory. Initial uncorrected artifacts are
not the final validated shader. See [earlier math history](../ADAPTATION_MATH_VALIDATION.md).

Fresh integration with final source-backed shaders passes all3 patterns /6
images: --strict-all-stages, full preprocessing --strict, and native JND checks.
Exports under validation/: spyr_spatial_17922614797262557 (asymmetric),
spyr_spatial_17922615112177470 (diagonal), spyr_spatial_17922615425028076 (impulse).
Evidence source_math_<pattern>_{run,stages,full,jnd}.log. No Vulkan validation
errors found. The generator's manifest-hash validation was rerun separately;
all4 generated .comp files are byte-identical to the tested final source files.

CSF precision and resize/masking remain separate validation tasks.

Further native-style power and source-backed log10 are composed by
../generate_power_shaders.ps1. power_reference.glsl keeps float-pair precision
across log/multiply/exp; it generates its own mathematical tables and reproduces
the observed native finite-result range guards. log10_reference.glsl uses the
pinned MIT log10_128_lead_tail_table.asm, including its split log10(2) tail, and
compensated FMA. No DLL is read. These additions implement the opt-in ABI7 map
backend's pupil/JND, masking/D and quality math without shaderFloat64.
For current measurements, failed candidates, scope and remaining CPU work see
[GPU map validation](../MAP_GPU_VALIDATION.md). General correctly rounded libm
behavior and cross-device equality are not claimed by these local tests.
