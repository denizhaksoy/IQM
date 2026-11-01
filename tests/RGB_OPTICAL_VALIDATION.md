# RGB optical/LMSR chain

2026-10-27: see `validation/rgb_optical_active_path.txt` for this checkpoint.
The `acceptance_v2` run passes **387,072 Float32 words bitwise**: six RGB96
images, three optical channels and four LMSR channels each. Fixtures are the
three nonidentity pairs described in `RGB_PROJECTION_VALIDATION.md` (official
3.0.7, Octave11.3,30ppd,age24,three display spectra).

Only original input planes enter the calculation. Each is explicitly transposed
from native column-major capture layout to the filter's row-major interface.
The GPU uses the accepted public v2 FFT192/IO assets with symmetric padding,
retained optical DC and the usual post-filter clamp. Output is transposed
back to column-major for comparison and projection. The matrix is independently
calculated from original cone/rod/display files using the C++ RGB calibration;
native optical/matrix intermediates are not injected. The previously accepted
FMA projection produces the pre-response-clamp LMSR planes.

`MixedFilterMode::OpticalRGB` accepts finite nonnegative primaries, including
zero. Existing optical luminance and local adaptation modes retain strict
positive input requirements. Checks confirm zero rejection for those modes
and negative rejection for RGB before GPU work. The public v4 luminance
`competing_axes` regression passes 69 checks / 507,149 values bitwise after
the production change. Vulkan validation was enabled; no reported errors.

The initial `acceptance` run failed on the first optical channel because the
test passed column-major values into the row-major filter interface. It is
retained. `acceptance_v2` corrects that boundary conversion; shader arithmetic
and tolerances were not changed.

Build target: `rgb-optical-gpu-test` in `tests/hdrvdp3_residual144`.
Arguments: native capture directory, public v2 `runtime/assets`, original
spectral data directory, accepted `rgb_projection.spv`, fresh output directory.
The source/build configuration uses `/fp:strict` on MSVC. Frozen executable,
source snapshot and hashes are kept with the checkpoint.

This validates RGB through pre-clamp LMSR, not through JND or quality scores.
Next connect LMSR response clamp, L+M adaptation, pupil reduction and separate
cone/rod JND responses. Then validate the complete RGB image-to-score chain
and expose its encoding/runtime interfaces. General dimensions, viewing
configurations and prediction modes remain outside this acceptance scope.

## Subsequent local adaptation checkpoint

`validation/rgb_jnd_active_path.txt` points to a fresh native `--jnd` capture
and `adaptation_acceptance` run. The original-input GPU chain now passes
497,664 Float32 words bitwise: the previous 387,072 optical/LMSR values plus
55,296 L+M input values and 55,296 local adaptation values. Cohorts overlap;
these are not independent extra image pairs. The new adaptation shader
clamps L and M separately before their sum. Existing local adaptation uses
replicate padding, log/FFT/filter/inverse/exp with retained DC; explicit
layout transposes bridge native planar columns and row-major filter input.

Reproduce native fixtures with `generate_rgb_projection_reference.m <fresh>
--jnd`. Compile `shaders/hdrvdp3/hdrvdp3_rgb_adaptation.comp` individually
with glslangValidator and validate its SPIR-V. Pass this SPIR-V as the optional
sixth argument to `rgb-optical-gpu-test` after its fresh output directory.
Pupil/JND/P outputs captured by the generator remain reference-only until
separately compared to GPU calculations. No full RGB quality claim.
