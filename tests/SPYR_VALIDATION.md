# SPYR numerical validation

Current extension:see [dimension validation](SPYR_DIMENSION_VALIDATION.md)
for27 raw-single cases across nine shapes,exact tail arithmetic and the
separate32x32 comparison-mask checkpoint. The original64x64 measurements
below retain their historical scope.

> Publication scope: this document describes locally validated work. The implementation, new test tools and full artifacts are not included in this documentation checkpoint. See SESSION_HANDOFF.md at the repository root for current status; earlier measurements below retain their historical scope.


Validated on 2026-10-16 against the local HDR-VDP reference reporting 3.07,
using GNU Octave 11.3.0, `matlabPyrTools/corrDn.m`, float32 JND inputs,
`buildSpyr(img, 3, 'sp0Filters', 'reflect1')`, and float32 pyramid output.
The official reference files were not edited.

The Vulkan harness uses reverse column tap traversal. The standard shader uses
explicit FMA; level 2 uses separate product/add rounding. Diagnostic scripts
identified these arithmetic paths by comparing every pixel at each stage.
The level-specific choice is validated for this 64x64 harness, not arbitrary sizes,
Octave builds, CPUs, or MEX implementations.

## Results

| Input | Images | Raw values compared | MaxAbsErr | RMSE | Mismatches |
|---|---:|---:|---:|---:|---:|
| Diagonal modulo pattern | ref + test | 19072 | 0 | 0 | 0 |
| Asymmetric modulo pattern | ref + test | 19072 | 0 | 0 | 0 |
| Edge and off-center impulses | ref + test | 19072 | 0 | 0 | 0 |

Each image has bands 64x64, 64x64, 32x32, 16x16, and 8x8.
DC-removed residuals also matched exactly for all six images. Comparisons parse
text back into float32, then compute error statistics in double precision.
The asymmetric and impulse JND inputs differ from their transposes.
The three Vulkan runs completed with exit code 0 and no VUID/validation error
messages were found in their logs.

This validates SPYR on identical exported JND inputs. It does not independently
validate luminance-to-JND preprocessing, pupil parameters, adaptation, CSF,
masking, or end-to-end quality scores. The built-in harness still prints legacy
comparisons against older C:/temp references; use the fresh comparison below.

## Reproduce in PowerShell from the workspace root

```powershell
cmake --build build-vs2026 --config Release --target IQM
$env:IQM_SPYR_PATTERN = 'asymmetric' # diagonal (default), asymmetric, or impulse
& './build-vs2026/bin/Release/IQM.exe' --method HDRVDP3 --enable-validation
# Use the unique validation directory printed by this invocation:
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet --no-gui tests/validate_spyr_spatial.m '<printed directory>'
Remove-Item Env:IQM_SPYR_PATTERN
```

The comparison checks shape, value count, finiteness, all five raw bands, and
the centered residual. Any mismatch causes an assertion and nonzero exit code.
GPU input/output files use row-major order; Octave band exports use column-major
order. The script explicitly converts between them.

Local evidence is under `validation/`: `spatial_final_comparison.log`,
`asymmetric_comparison.log`, and `impulse_comparison.log`, with corresponding
GPU run logs and unique `spyr_spatial_*` export directories.
