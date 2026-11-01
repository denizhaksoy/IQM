# Double192 GPU convolution composition

Validated 2026-10-29. Completed artifact pointer:
`validation/double192_chain_completed_path.txt`.

The diagnostic now executes six GPU dispatches without supplying CPU
intermediates: real forward axis, complex forward axis, Double spectral
multiplication, inverse first axis, normalized real inverse second axis,
and96x96 top-left crop. Input is the official192x192 padded Double spatial
boundary. No native FFT or filtered-spectrum values are used as GPU inputs.

Filter coefficients are generated on CPU from existing `generate_mtf_filter`
and the reference Gaussian equation, not loaded from native coefficient files.
Native coefficients are read only for a separate bitwise comparison. This is
GPU convolution composition, not an assertion that coefficient preparation,
image padding, spectral projection, clamp, adaptation exp or quality are GPU.

Three cases (two distorted pairs and an identity control), three display
spectra, two image sides and four convolutions cover72matrices. Profile:
96sq source,192sq padding,30ppd,age24,defaultsp0, symmetric optical/replicate
adaptation postpadding inherited from native inputs. Column-major complex
Double throughout, DC unchanged, inverse divisor36864 with tested FMA
remainder correction. NVIDIA RTX A500 Laptop GPU; explicit Float64/RTE64/
SignedZeroInfNanPreserve64. DenormPreserve64 unavailable; no generic guarantee.

- 2,654,208 complete inverse real Double words: zero bit differences.
- 663,552 GPU-cropped Double words: zero bit differences (overlapping scope).
- All source-generated spectral filter coefficients: zero bit differences.
- All accepted Vulkan validation logs: no VUID/[ERROR].

The existing accepted forward/inverse shader binaries are reused; their
paths are recorded in lineage.json. New multiplication/crop shaders are
compiled to fresh directories with `compile_double192_multiply.ps1`; supply
`-ShaderSource tests/double192_crop.comp` for crop. No shaders_out deletion or
production changes. Build target `double192-chain-gpu-test` in
`tests/hdrvdp3_double_gpu` uses strict FP settings.

```text
double192-chain-gpu-test <native-convolution-dir> <forward-first.spv> <forward-second.spv> <multiply.spv> <inverse-first.spv> <inverse-second.spv> <crop.spv> <fresh-output>
node tests/hdrvdp3_evaluation/audit_double192_chain.cjs <root>
```

The audit verifies output bytes against actual official inverse and crop
captures, coverage counts, source-filter check results and validation logs.
The new harness is separate; earlier FFT harness behavior is unchanged.

Next: replace native padded inputs with independently prepared Double optical
and adaptation inputs, compose post-convolution spectral projection/clamp and
adaptation operations, and validate resulting visual boundaries. Remaining
Double pyramid, masking, pooling, production runtime and CLI GPU integration
still need work; this milestone does not complete the whole metric.
