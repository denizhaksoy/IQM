# Single mutual-activity GPU validation — 2026-11-05

This is an additional experimental GPU stage. It does not resolve the earlier
official-reference football150 anomaly or establish universal HDR-VDP-3 parity.

## Preserved CPU operation

`mutual_mask_single` exposes the existing strict CPU stage for direct validation.
It takes the minimum absolute reference/test band activity and applies the 3x3
correlation. The canonical transpose for landscape inputs, zero padding,
fx-ascending/fy-descending loop order, and distinct FMA processing of the final
32-row tail are preserved. Inputs/outputs are row-major Single band tensors.
No FFT, frequency-domain padding, DC processing or image resizing occurs here.

`mutual_single_gpu` uses three separate storage buffers, validates finite input
geometry and Vulkan ranges, and explicitly requires enabled shaderFloat64.
It has no CPU arithmetic fallback. The standalone runtime accepts up to 4K
pixel count and dimensions no greater than 16,384 each.

## Rejected prototypes and correction

- `validation/mutual_gpu_acceptance_20261105_run01`: native Float32 shader on
  Intel Arc Pro failed the 1x65 synthetic case (10/65 bit mismatches). This
  prototype is rejected, not a tolerance pass.
- `run02`: the native shader on the Float64-capable NVIDIA profile matched the
  six real bands but failed all 1,089 values of the subnormal probe. This whole
  run is also rejected.
- The corrected shader decodes subnormal inputs through integer bits, computes
  exact Float32 products in Double, preserves the addition residual with TwoSum,
  and rounds explicitly to Float32 RTE. Non-fused tail operations round the
  product separately. Subnormal output is constructed through integer bits,
  preventing a native arithmetic flush. Double-addition residuals retain the
  direction of ties at Float32 rounding boundaries.

Old logs, result directories and the native prototype SPIR-V are preserved.
The corrected shader is `mutual_rounding.spv`, separate from `mutual.spv`.
Acceptance currently applies to the measured NVIDIA RTX A500 Laptop GPU;
no corrected Intel acceptance is claimed (the new path requires enabled FP64).

## Standalone acceptance

`validation/mutual_gpu_acceptance_20261105_run04/acceptance.json` passed:
**35 cases, 19,479,447 values, zero bit differences** against the strict CPU
stage. Coverage includes portrait/landscape, partial workgroups, 32-row FMA
tails, zeros, random values across the finite Float32 exponent range, normal/
subnormal boundaries, largest finite inputs, and all six real 4K pyramid bands
from the later football150 reference capture. A passing earlier corrected run03
is preserved; repeated real inputs are not additional independent coverage.

## Quality-stage integration

An explicit optional `SingleMutualBackend` runs once per band; invalid output or
provider failure is rejected without fallback. Existing default CPU computation
is retained. Quality isolation passed in
`validation/mutual_quality_acceptance_20261105_run01/acceptance.json`:
six mutual GPU dispatches plus one comparison-mask dispatch, with all six band
errors, Double base frequency and Q/Q_JOD bitwise identical to the CPU path.
Q=40ec7071 and Q_JOD=41038aea match the later saved clean reference. PPD is
62.729130063099447; unchanged Single FFTW is three-thread estimate/cleared wisdom
and base FFT DC zeroing remains in the CPU quality stage.

This isolation test uses persisted reference stages, explicitly transposed from
column-major to row-major. It does not claim independent end-to-end validation.

The separate `IQM-hdrvdp3-mutual-experimental` application target requires
`--projection-shader`, `--comparison-shader` and `--mutual-shader`. It reports
backend `experimental-vulkan-matrices-comparison-mutual-cpu-fftw` and counts each
GPU stage separately in `gpu_stages.json`; `full_gpu` remains false.
The independent PNG-to-score wrapper is
`tests/run_live_hdr_mutual_experimental.cjs`. The football150 run PASSED under
`validation/live_hdr_mutual_e2e_20261105_run01`: Q=40ec7071, Q_JOD=41038aea,
four matrix, one comparison and six mutual dispatches, no reported Vulkan
validation errors. Only original RGB16 PNGs, model files, FFTW and shaders were
supplied; no reference intermediate tensors were supplied to this execution.

The 27-video representative frame0 regression is running separately through
`tests/audit_live_hdr_mutual_backend.cjs`; its checkpoint and frozen runtime are
under `validation/live_hdr_mutual_backend_audit_20261105_run01`. Existing CPU/
hybrid and comparison-mask-only audits remain independent and unchanged.

`tests/continue_live_hdr_mutual_frames.cjs` is also running, initially waiting
for that representative regression to complete. If it passes, this continuation
freezes and verifies the same runtime/model and recomputes every one of the
513 accepted phase0/stride30 image pairs with two workers, comparing score bits
to the preserved official records. It then checks native Single video means.
It does not recompute or replace official predictions and retains the historical
anomaly flag. A failed worker stops scheduling and drains in-flight work before
the final failure report. Results are in
`validation/live_hdr_mutual_all_frames_20261105_run01`. Total frame count remains
zero in its waiting checkpoint until the verified input plan has been built.
The upstream-failure blocking guard passed in
`validation/live_hdr_mutual_frame_guard_20261105_run01`; its deliberately failed
fixture is not a failure of the real continuation.

## Reproduction

Build `mutual-probe` from `tests/comparison_single_gpu/CMakeLists.txt`, compile
`shaders/hdrvdp3/hdrvdp3_mutual_single.comp` with glslc for Vulkan1.2 into a fresh
output directory, and validate SPIR-V with spirv-val. The probe accepts
`shader fresh-result-directory [4K-capture-root]`. It exits nonzero on any bit
mismatch, including subnormals. Never run compile_shaders.sh for these tests.
