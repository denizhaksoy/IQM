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

Remaining: full forward128x128 composition, the distinct final two-frequency
graph, two-axis Hermitian completion, experimental production integration,
strict full-chain preprocessing, log/exp/subnormal behavior, and full metric
validation. The tail graph itself is verified; its placement in the complete
forward transform is still pending.
