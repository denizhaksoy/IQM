# Publication validation, 2026-10-16

Scope: isolated test tools and FFTW-derived arithmetic fixtures only. No
production metric, preprocessing, default setting, submodule or root build
change is included in this checkpoint.

Fresh standalone Release build succeeded with Visual Studio 2026/MSVC and
Vulkan SDK 1.4.363.0. The PowerShell generator and glslc rebuilt the shader
from the included source. Native oracle: installed Octave 11.3.0 single FFTW
DLL. GPU: Intel Arc Pro Graphics, Vulkan validation enabled.

All eight fresh synthetic test groups passed strict selected-path comparison:

| Group | Selected graph | Transforms | Complex outputs | Max error / RMSE / mismatches |
|---|---|---:|---:|---|
| Inverse leaves 8/16 and composed 128 | 1 | 512 per size | 77824 | 0 / 0 / 0 |
| Normalized inverse 128x128 | 3 then 4 | 7 matrices | 114688 | 0 / 0 / 0 |
| Forward real128 | 5 | 512 | 65536 including conjugates | 0 / 0 / 0 |
| Inverse second-axis group6 | 2 | 516 | 66048 | 0 / 0 / 0 |
| Inverse second-axis group2 | 2 | 512 | 65536 | 0 / 0 / 0 |
| Forward second-axis group65 | 6 | 520 | 66560 | 0 / 0 / 0 |
| Forward second-axis group3 | 6 | 513 | 65664 | 0 / 0 / 0 |
| Forward final group2 | 7 | 512 | 65536 | 0 / 0 / 0 |

No Vulkan validation errors were found. Diagnostic nonselected graph/FMA
variants may intentionally differ; success requires the explicitly selected
graph, not any passing candidate. Half-spectrum conjugates are not independent
R2C outputs. This is numerical equality on fixtures, not universal bitwise
equivalence or a full HDR-VDP-3 validation claim.

Local fresh-build evidence (not shipped binaries):
`validation/fft_publish_final_build.log` and
`validation/fft_publish_checks_639277456979720179/` with eight separate oracle
and GPU logs. Synthetic fixtures can be regenerated without local snapshots;
see [build and run instructions](README.md). Earlier native Octave checks,
spatial snapshot comparisons and provenance are in
[the detailed validation record](../fftw_reference/README.md).

At this checkpoint full forward composition remained pending. The subsequent
verification below completes that isolated test scope.

## Complete forward128x128 validation, 2026-10-17

The standalone Release tools now compose R2C, main/tail SIMD routines and
two-axis Hermitian completion. All19 real matrices (7 synthetic and12 existing
MTF/adaptation stage0 snapshots across3 patterns) match exactly: 311296
full-spectrum complex values, max error0, RMSE0, mismatches0. This includes
Hermitian reconstruction; 158080 compact bins are independently computed.
Every CPU expected matrix independently matches native single Octave fft2,
with actual native single thread count22 verified through the extension.
No Vulkan validation errors found.

Final rebuilt tools also pass fresh synthetic forward2d and inverse2d fixtures,
plus existing R2C, forward main and distinct tail regressions. Evidence:
`validation/fft_forward2d_final_build.log` and
`validation/fft_forward2d_final_639278562885963009/`. Detailed19-matrix artifacts
and commands are documented in `../fftw_reference/README.md`.

Production integration, strict full-chain preprocessing, log/exp/subnormal
behavior and full metric validation remain outstanding. These isolated FFT
passes do not establish full HDR-VDP-3 parity.
