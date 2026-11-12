# Proposed delivery commit contents (NOT staged or committed)

Audit boundary: current `feature/hdr-vdp3` working tree versus HEAD. Existing
staged/unstaged changes and untracked files remain intact. Use an explicit path
allowlist after review; `git add .` or `git commit -a` would include unrelated work.
This file is a review proposal, not authorization to commit or push.

## Include: delivery source/configuration

```text
CMakeLists.txt
bin/CMakeLists.txt
bin/IQM-hdrvdp3-cpu/main.cpp
bin/IQM/args.cpp
bin/IQM/args.h
bin/IQM/main.cpp
bin/IQM/hdrvdp3_live_quality.cpp
bin/IQM/hdrvdp3_live_quality.h
bin/shared/hdrvdp3_single_cli.cpp
bin/shared/hdrvdp3_single_cli.h
cmake/HDRVDP3Delivery.cmake
tools/hdrvdp3_delivery_model_hashes.json
tools/snapshot_hdrvdp3_delivery.cjs
tools/package_hdrvdp3_delivery.cjs
tools/package_hdrvdp3_regression.cjs
tools/hdrvdp3_cpu_baseline/CMakeLists.txt
tools/hdrvdp3_cpu_baseline/main.cpp
```

## Include: verification/benchmark scripts

```text
tests/verify_hdrvdp3_live_cli.cjs
tests/verify_hdrvdp3_clean_delivery.cjs
tests/verify_hdrvdp3_delivery_smoke.cjs
tests/freeze_hdrvdp3_live_acceptance.cjs
tests/run_hdrvdp3_delivery_acceptance.cjs
tests/report_hdrvdp3_delivery_acceptance.cjs
tests/watch_hdrvdp3_delivery_acceptance.cjs
tests/benchmark_hdrvdp3_delivery.cjs
tests/audit_hdrvdp3_delivery.cjs
```

## Include: delivery documentation and session record

```text
tests/HDRVDP3_DELIVERY_PROFILE.md
tests/HDRVDP3_CLI_INTEGRATION_2026-11-11.md
tests/HDRVDP3_CLEAN_DELIVERY_2026-11-11.md
tests/HDRVDP3_LIVE_ACCEPTANCE_SELECTION_2026-11-12.md
tests/HDRVDP3_FINAL_DELIVERY_2026-11-12.md
tests/HDRVDP3_DELIVERY_COMMIT_REVIEW_2026-11-12.md
SESSION_HANDOFF.md
```

Earlier checkpoint documents retain their historical scope; the final report
states the later acceptance/benchmark state. AGENTS.md is a separate local
workflow policy addition: preserve it, but exclude from this delivery commit
unless explicitly reviewed as repository policy.

## Shader audit

No changed/new shader source is required for this delivery commit. These six
delivery dependencies are already tracked and unchanged versus HEAD:

```text
shaders/hdrvdp3/hdrvdp3_rgb_projection_2d.comp
shaders/hdrvdp3/hdrvdp3_comparison_single.comp
shaders/hdrvdp3/hdrvdp3_mutual_single.comp
shaders/hdrvdp3/hdrvdp3_comparison_resize.comp
shaders/hdrvdp3/hdrvdp3_adaptation_input_single.comp
shaders/hdrvdp3/hdrvdp3_padding_single.comp
```

Existing tracked numerical C++/headers, stb and decode/official scoring/clock-grid
helpers remain dependencies, not newly changed commit candidates. They are
included in source-snapshot/provenance checks. Do not modify their arithmetic.

## Exclude and preserve locally

- Dirty `lib/VkFFT` submodule and untracked `lib/glslang-main/`.
- Pre-existing staged AND unstaged `shaders/hdrvdp3/hdrvdp3_corrdn_test.glsl`:
  experimental, not used by the delivery; do not unstage/reset it automatically.
- `tests/FFT4K_SCALAR_LEAF_VALIDATION.md`, `tests/fft_plan_single/CMakeLists.txt`,
  `tests/inspect_fft4k_subplans.cpp` changes: frozen manual FFT experiments.
- `tests/fft7680_forward_gpu.cpp`, `.h`, `tests/fft7680_forward_gpu/`,
  `tests/fft7680_gather4.comp`, `tests/fft_t3fv20_gpu.cpp`, `.h`,
  `tests/fft_t3fv20_gpu/`, `tests/generate_fft_t3fv20_shader.cjs`,
  `tests/inspect_fft7680_native_fixtures.cpp`,
  `tests/verify_fft7680_forward_gpu.cpp`, `tests/verify_fft_t3fv20_gpu.cpp`.
- `bin/IQM-test/backup_local_adapt_fft_test.cpp`, shader `.backup`/`.spvasm`
  files (`original*`, `precise`, `test`) and all `shaders_out/hdrvdp3/` binaries.
- Root scratch files `full_output.txt`, `jnd_cpp.csv`, `jnd_sampling_cpp.csv`,
  `la_fft_output.txt`, `outfile.txt`, `output.txt`, `output_full.txt`.
- Every `validation/` generated result, PNG/MP4/dataset, downloaded official
  implementation, local frozen source/build/package, benchmark logs/PE/DLL/SPIR-V,
  temporary output and machine-specific absolute-path manifest.
- Existing build directories and all other generated binaries/large archives.

Preserve large evidence outside Git with original hashes. The concise final
report records acceptance bits, exact profile, package/manifest IDs and benchmark
measurements; scripts reproduce the evidence given the documented prerequisites.
No official reference source, datasets or third-party DLL is proposed for Git.

## Pre-commit review gate

Verify the explicit list against fresh `git status --short` and `git diff` after
review. The index already contains unrelated shader work: a normal commit of
everything staged would be incorrect. Agree a path-limited commit/index strategy
at commit authorization time without discarding that existing index content.
No staging, commit, push, reset, clean or submodule mutation has been performed.
