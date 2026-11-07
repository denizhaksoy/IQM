# Experimental Single comparison-mask GPU stage

2026-11-05. This stage does not establish full GPU HDR-VDP-3 or reproduce
the original paper's private evaluation cohort.

The standalone implementation uses three separate storage buffers. At 4K,
each RGB buffer is 99,532,800 bytes and the mask is 33,177,600 bytes.
Storage/dispatch limits are checked before allocation. The caller must explicitly
enable shaderFloat64; the runtime rejects a device instance without that feature.
Inputs are three positive finite planar Float32 native RGB channels with the
existing native front-end floor of 1e-6f. Output preserves the supplied pixel
order and consists of Float32 0/1 values. There is no image resizing, padding,
FFT or DC processing in this kernel.

## Exact threshold semantics

The CPU expression is `abs(test-reference)/reference > 0.001f`, with subtraction
and division rounded to Float32. The shader preserves the Float32 subtraction
and compares its absolute result to an exact Double product of the reference
and the upper rounding midpoint of 0.001f. The threshold's Float32 bits are
0x3a83126f (odd significand); the midpoint ties to the next even Float32, so the
test uses `>=`. The midpoint has at most 25 significant bits and a Float32
reference has 24; their product fits exactly in the 53-bit Double significand.
This avoids relying on approximate shader Float32 division near the threshold.
The accepted native domain keeps nonzero differences away from subnormals.

## Measured standalone result

`validation/comparison_gpu_acceptance_20261105_run01/results.jsonl`:

| Case | Pixels | Bit mismatches |
| --- | ---: | ---: |
| Near-threshold values, each channel, both signs | 999,995 | 0 |
| Identical inputs and partial workgroup | 65 | 0 |
| Native-domain extremes and partial workgroup | 65 | 0 |
| Preserved football150 native RGB tensors, 3840x2160 | 8,294,400 | 0 |

Total: 9,294,525 mask values, bitwise agreement with the strict C++ expression.
Vulkan validation was enabled; no validation errors were reported in the log.
Device in this run: NVIDIA RTX A500 Laptop GPU. Real tensors are the column-major
planar exports of the later matching reference capture, not a reconstruction of
the historical discrepant run. Both inputs have the same layout, so the mask
comparison preserves that layout. No claim covers arbitrary devices or all inputs.

A second standalone run also passed all 8,294,400 pixels of the separate
`validation/live_hdr_capture_20261101_run03/results` native capture, plus the
same synthetic checks. Its results are under
`validation/comparison_gpu_acceptance_20261105_run02`. Repeated synthetic cases
are not additional independent input coverage. Source, shader, binary and input
SHA-256 records are in
`validation/comparison_gpu_shader_20261105_run01/provenance.json`.

## Build and run

```powershell
cmake -S tests/comparison_single_gpu -B validation/comparison_gpu_build_20261105_run01 -G "Visual Studio 18 2026" -A x64
cmake --build validation/comparison_gpu_build_20261105_run01 --config Release
glslc --target-env=vulkan1.2 shaders/hdrvdp3/hdrvdp3_comparison_single.comp -o validation/comparison_gpu_shader_20261105_run01/mask.spv
spirv-val --target-env vulkan1.2 validation/comparison_gpu_shader_20261105_run01/mask.spv
```

Create a fresh shader-output directory before compilation, and a fresh result
directory for every execution. The standalone `comparison-probe` accepts
`shader fresh-output [reference-native.f32 test-native.f32]`.

`comparison-quality` additionally checks the optional `SingleComparisonBackend`
inside the quality stage using saved reference adaptation/pyramid/native tensors.
It transposes each plane from column-major exports to the quality API's row-major
layout and compares CPU/GPU-provider band errors, base frequency and final score
bits. It is an isolation test, not an independent end-to-end implementation.
Its arguments are `capture-root shader fftw-single-dll ppd fresh-output` (4K).

Quality-stage isolation also PASSED in
`validation/comparison_quality_acceptance_20261105_run01/acceptance.json`:
all six band errors and the Double base frequency were bitwise identical;
Q was `40ec7071`, Q_JOD was `41038aea`, matching the preserved later reference
capture's score bits. One comparison-mask GPU dispatch was executed. PPD was
62.729130063099447, Single FFTW used three threads with estimate/cleared wisdom,
and the base FFT DC coefficient was zeroed by the unchanged quality code.
The original earlier outlier (`40ec4f50` / `41037c75`) remains unresolved.

Default CPU quality and the existing public four-matrix-dispatch hybrid profile
continue to use the CPU comparison mask. The optional provider is explicit,
validates binary output, and does not silently fall back after a provider error.
Public application support for this new experimental stage still requires an
explicit backend profile and independent end-to-end regression validation.

## Experimental application integration, 2026-11-05

The separate `IQM-hdrvdp3-mask-experimental` target now requires both
`--projection-shader` and `--comparison-shader`. It explicitly enables Float64,
reports backend `experimental-vulkan-matrices-mask-cpu-fftw`, and exports
`gpu_stages.json` with four matrix dispatches, one mask dispatch and
`full_gpu:false`. Existing CPU and four-matrix hybrid profiles remain separate.

An independent PNG-to-score run passed on football150:
`validation/live_hdr_mask_e2e_20261105_run01/comparison.json`.
It supplied only the original decoded RGB16 PNGs, four model files, FFTW and
the two shaders. No reference intermediates were supplied to the executable.
Q=40ec7071 and Q_JOD=41038aea match the later clean stock reference. Vulkan
validation reported no errors. The original discrepant score is still retained.

The executable/shaders were frozen for the run; all model/image/runtime hashes
were checked before and after execution. The wrapper is
`tests/run_live_hdr_mask_experimental.cjs`. A separate representative frame0
regression across all 27 accepted local videos is running through
`tests/audit_live_hdr_mask_backend.cjs`, with frozen executable/shaders/model
files and checkpoint at
`validation/live_hdr_mask_backend_audit_20261105_run01/checkpoint.json`.
It stops on a score, profile, provenance or Vulkan validation mismatch. It does
not replace full-video validation or add new unique image-pair coverage.
