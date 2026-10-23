# Rectangular full-quality stress validation

Accepted checkpoint: `validation/rectangular_stress_639282921084448635`;
completed pointer: `validation/rectangular_stress_completed_path.txt`.
All six new pairs pass all 36 exact stage validators and native score bits.
Combined with the prior checkpoint, eight unique full rectangular pairs pass.

| Height x width | Input / transform | Q | Q_JOD |
| --- | --- | --- | --- |
| 64 x 128 | impulse / localized | 2.7487683296203613 | 3.4179229736328125 |
| 64 x 128 | HDR ramp / localized | 8.3044567108154297 | 8.9771928787231445 |
| 64 x 128 | asymmetric / identity | 10 | 10 |
| 128 x 64 | impulse / localized | 2.3844747543334961 | 2.9913034439086914 |
| 128 x 64 | HDR ramp / localized | 8.4348869323730469 | 9.0768861770629883 |
| 128 x 64 | asymmetric / identity | 10 | 10 |

Both identity pairs have five zero band errors and zero aggregate error.
No numerical engine or shader changes were needed. This session changes fixture
generation, orchestration and source-based runtime packaging. Existing public
square regressions retain the acceptance recorded in the previous checkpoint;
they were not rerun against unchanged engine/shader binaries in this session.

This continues [the ABI16 checkpoint](RECTANGULAR_MAPS_VALIDATION.md) with
fresh, independent official HDR-VDP-3 v3.0.7 references and GPU exports for both
64x128 and 128x64. Each direction covers:

- Impulse luminance input with a localized `reference*1.4+0.25` change.
- HDR ramp from 0.001 to 10,000 cd/m² with the same localized change.
- Identical asymmetric reference/test images, expecting zero error and
  `Q=Q_JOD=10`.

Run `tests/hdrvdp3_evaluation/validate_rectangular_stress.ps1` from the workspace
root with a fresh `-OutputDirectory`. The fixture generator accepts optional
`localized` (default) or `identity` as its third argument. The script compares
score bits and runs all six exact validators per pair. A case enters
`results.json` only after every check passes; `PASS.txt` requires all six cases.
The active/completed pointers under `validation/` identify the current artifact
directories. Partial runs must not be interpreted as complete validation.

Native and GPU use the same Float32 luminance pixels, 30ppd, age24, D65 and
matching dimensions. Native FFTW uses 3 threads, estimate planning and fresh
single wisdom before independent full chains. Maps are exported row-major.
MTF uses symmetric post-padding; LA uses replicate post-padding. The FFT path
uses explicit column-major spectra/filter transpose and inverse scaling once
by 1/32768. Residual FFTs are unpadded 8x16/16x8, with native mean removal,
DC magnitude zeroing and first-max selection. Binary comparison masks resize
in binary64. Numeric equality is exact; Float32 signed-zero bit identity is
not asserted. Official score comparison is bitwise.

The six rectangular diagnostic runtime shaders can now be regenerated as one
package using `generate_assets_rectangular.ps1 -OutputDirectory <fresh-dir>
-VulkanBin <SDK/Bin>`. Every binary must match
`REFERENCE_SHADER_HASHES_RECTANGULAR.json`. The package includes compiler and
ABI metadata plus FFTW/AMD licenses, and regenerates sources without depending
on earlier validation output directories.

This is local Intel Arc diagnostic coverage. Public quality dimensions remain
square; this checkpoint does not enable rectangular public API, RGB, arbitrary
dimensions, other viewing parameters or other GPU devices.
