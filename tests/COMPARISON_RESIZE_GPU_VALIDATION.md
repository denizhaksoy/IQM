# Double comparison-mask resize GPU stage — 2026-11-06

The comparison mask in the Single quality profile is Double. The new optional
`SingleComparisonResizeBackend` keeps that type, the existing bicubic axis plans,
symmetric edges, horizontal-then-vertical passes, tap accumulation order and
normalization. CPU prepares coefficients/indices; image multiplication, addition
and division execute on Vulkan. Identity axes skip dispatch and preserve input.

The runtime requires enabled shaderFloat64, checks storage/dispatch limits before
allocation and accepts binary input masks with source/target/intermediate counts
up to 3840x2160. Input, axis-plan and output storage buffers are separate. There
is no FFT, DC processing or CPU arithmetic fallback in this resize stage.
Masks use row-major little-endian Float64 on the measured host/device; the
standalone real fixture explicitly transposes the preserved column-major
native exports before forming the binary mask. Existing symmetric edge-index
folding is preserved; no additional image padding, cropping or DC treatment is
introduced. Axis plans store exact source indices and Double weights/normalizers.

## Measured acceptance

- Standalone: **26 cases / 11,068,767 Double values**, bitwise identical to the
  existing CPU resize. Coverage includes up/down resizing, odd dimensions,
  single-axis/identity passes, zero/one/random binary masks and real 4K masks.
  Identity cases are included in the comparison count and use zero dispatches.
- Quality isolation: six calls/eight resize dispatches on the 4K six-band
  profile; six band errors, base frequency and Q/Q_JOD match the CPU path.
  Q=40ec7071, Q_JOD=41038aea. This uses saved reference stages, not independent
  end-to-end evaluation.
- Independent PNG-to-score football150 run: exact reference Q/Q_JOD bits, four
  matrix + one comparison + six mutual + eight resize dispatches, no reported
  Vulkan validation errors. No reference intermediates supplied to the app.
- New resize-enabled application profile: **14/14 synthetic setting cases**
  matched fresh clean official v3.0.7 and the CPU application, covering small/
  odd/portrait/landscape dimensions, PPD26–256 and seven/eight-band pipelines.
  These contain12 distinct PNG pairs; repeated PPD settings are not new images.

Measured device: NVIDIA RTX A500 Laptop GPU. This is measured finite coverage,
not universal bitwise identity across devices/inputs. Native Double shader
division passed these tests; no tolerance pass is reported as exact.

## Application and API

The existing mutual experimental target accepts the explicit optional flag
`--comparison-resize-shader PATH`. With it, backend is
`experimental-vulkan-matrices-comparison-mutual-resize-cpu-fftw`. GPU-stage JSON
reports resize calls/dispatches separately. Other profiles keep their previous
arithmetic, and `full_gpu` remains false. An empty provider, invalid shape or
nonfinite returned mask is rejected. Bicubic ringing may produce finite values
outside0–1, so provider validation does not incorrectly clamp these values.

Both `evaluate_quality_single` and the full in-memory
`evaluate_general_quality_single` accept the optional provider. Other resizing
operations, FFT, adaptation, JND, CSF and remaining quality arithmetic still use
CPU processing; this is not a full GPU metric.

## Artifacts

- `validation/comparison_resize_acceptance_20261106_run01`
- `validation/comparison_resize_quality_20261106_run01`
- `validation/live_hdr_resize_e2e_20261106_run01`
- `validation/hdrvdp3_resize_profile_matrix_20261106_run01`
- `validation/comparison_resize_runtime_20261106_run01` (frozen app/shaders/model)
- `validation/comparison_resize_shader_20261106_run01` (shader/build/run logs)

Standalone build: `tests/comparison_resize_gpu/CMakeLists.txt`. Compile only
`shaders/hdrvdp3/hdrvdp3_comparison_resize.comp` into a fresh output directory;
never use compile_shaders.sh for this work. The quality test accepts the resize
shader after the mutual shader. Profile-matrix runner accepts `all resize` after
the fresh output path to explicitly enable the new backend.

## Broader real-mask stage sweep completed (2026-11-07)

`run_comparison_resize_live_sweep.cjs` verifies the accepted513+16 source image
pairs and the passed resize-enabled matrix, freezes the new stage probe/shader/
model, then compares every changed Double mask output on all529 distinct real
inputs. It recomputes native masks from original RGB16 PNGs using the established
CPU PQ/color preprocessing and checks all four downsampled target masks. This
isolates the changed stage without rerunning unrelated FFT/JND computations.
It produces no new full quality predictions and must not be described as a
529-frame end-to-end test of the resize-enabled profile.

The original run stopped after340 exact frames. All original artifacts remain
unchanged. The frozen probe resumed the remaining189 in a fresh directory via
`tests/continue_comparison_resize_live.cjs`. Combined acceptance PASSED529 unique
frames /1,456,866,000 Double values, with exact bit comparisons,8 dispatches per
frame, input/model/runtime preservation and no reported Vulkan validation errors.
Final evidence: `validation/comparison_resize_live_continuation_20261107_run01/completion_audit.json`.
Original prefix: `validation/comparison_resize_live_20261106_run01/results/results.jsonl`.
Continuation: `validation/comparison_resize_live_continuation_20261107_run01/results/results.jsonl`.
Prior official score files and numerical exports are retained unchanged.

The historical original-reference football150 discrepancy and the original
paper's cohort/protocol gaps remain open; this work does not resolve them.
